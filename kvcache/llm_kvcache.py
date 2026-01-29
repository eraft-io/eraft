# 
# pip install torch numpy tqdm redis
import torch
import torch.nn as nn
import torch.optim as optim
from torch.nn import functional as F
from torch.utils.data import DataLoader, Dataset
import numpy as np
from tqdm import tqdm
import redis
import pickle
import math
import uuid

# ----------------------------
# 全局配置
# ----------------------------
vocab_size = 1000
d_model = 2048          # 增大模型维度 → 更大 KV Cache
nhead = 16
num_layers = 24         # 增加层数 → 更多缓存张量
max_seq_len = 512
batch_size = 2
epochs = 1
device = 'cuda' if torch.cuda.is_available() else 'cpu'

# Redis 分片配置
CHUNK_SIZE = 2        # 每片最多 256 个 tokens
MAX_CONTEXT_LEN = 1024  # 最大缓存长度（防无限增长）
TTL_SECONDS = 6000       # 10 分钟过期

# 初始化 Redis 连接
redis_client = redis.Redis(host='localhost', port=6379, db=0, decode_responses=False)

# ----------------------------
# 张量分片存储/加载（核心修复）
# ----------------------------
def save_tensor_sharded(session_id, layer_idx, tensor_name, tensor, ttl_seconds=TTL_SECONDS):
    """将 (B, H, S, D) 张量按 S 维度分片存入 Redis"""
    B, H, S, D = tensor.shape
    num_chunks = math.ceil(S / CHUNK_SIZE)
    
    # 存元数据
    meta_key = f"kv:{session_id}:l{layer_idx}:{tensor_name}:meta"
    redis_client.hset(meta_key, mapping={
        b"B": str(B),
        b"H": str(H),
        b"S": str(S),
        b"D": str(D),
        b"chunks": str(num_chunks)
    })
    redis_client.expire(meta_key, ttl_seconds)
    
    # 存分片
    for i in range(num_chunks):
        start = i * CHUNK_SIZE
        end = min((i + 1) * CHUNK_SIZE, S)
        chunk = tensor[:, :, start:end, :]
        chunk_key = f"kv:{session_id}:l{layer_idx}:{tensor_name}:chunk_{i}"
        redis_client.set(chunk_key, pickle.dumps(chunk.cpu()), ex=ttl_seconds)

def load_tensor_sharded(session_id, layer_idx, tensor_name, device):
    """从 Redis 分片读取并拼接张量"""
    meta_key = f"kv:{session_id}:l{layer_idx}:{tensor_name}:meta"
    meta = redis_client.hgetall(meta_key)
    if not meta:
        return None
    
    try:
        B = int(meta[b'B'])
        H = int(meta[b'H'])
        S = int(meta[b'S'])
        D = int(meta[b'D'])
        num_chunks = int(meta[b'chunks'])
    except (KeyError, ValueError):
        return None

    chunks = []
    for i in range(num_chunks):
        chunk_key = f"kv:{session_id}:l{layer_idx}:{tensor_name}:chunk_{i}"
        data = redis_client.get(chunk_key)
        if data is None:
            return None
        chunk = pickle.loads(data).to(device)
        chunks.append(chunk)
    
    return torch.cat(chunks, dim=2)

def get_past_kv_from_redis(session_id, num_layers, device):
    """从 Redis 获取历史 KV Cache（修复：新增 device 参数）"""
    past_key_values = []
    for i in range(num_layers):
        k = load_tensor_sharded(session_id, i, "key", device)
        v = load_tensor_sharded(session_id, i, "value", device)
        if k is None or v is None:
            return None
        past_key_values.append((k, v))
    return past_key_values

def save_past_kv_to_redis(session_id, past_key_values, ttl_seconds=TTL_SECONDS):
    """将 KV Cache 分片保存到 Redis"""
    for i, (k, v) in enumerate(past_key_values):
        # 截断超长上下文
        if k.size(2) > MAX_CONTEXT_LEN:
            start = k.size(2) - MAX_CONTEXT_LEN
            k = k[:, :, start:, :]
            v = v[:, :, start:, :]
        save_tensor_sharded(session_id, i, "key", k, ttl_seconds)
        save_tensor_sharded(session_id, i, "value", v, ttl_seconds)

# ----------------------------
# 数据集（不变）
# ----------------------------
class QADataset(Dataset):
    def __init__(self, data, vocab_size, max_len):
        self.data = data
        self.vocab_size = vocab_size
        self.max_len = max_len

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        seq = self.data[idx]
        if len(seq) > self.max_len:
            seq = seq[:self.max_len]
        else:
            seq = seq + [0] * (self.max_len - len(seq))
        return torch.tensor(seq, dtype=torch.long)

np.random.seed(42)
train_data = []
for _ in range(500):  # 减少数据量加速测试
    q_len = np.random.randint(5, 15)
    a_len = np.random.randint(3, 10)
    question = np.random.randint(10, vocab_size//2, q_len).tolist()
    answer = np.random.randint(vocab_size//2, vocab_size, a_len).tolist()
    seq = [1] + question + [2] + answer + [3]
    train_data.append(seq)

dataset = QADataset(train_data, vocab_size, max_seq_len)
dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True)

# ----------------------------
# 模型（不变）
# ----------------------------
class TransformerQAModel(nn.Module):
    def __init__(self, vocab_size, d_model, nhead, num_layers, max_seq_len):
        super().__init__()
        self.d_model = d_model
        self.num_layers = num_layers
        self.embedding = nn.Embedding(vocab_size, d_model)
        self.pos_encoding = self._create_positional_encoding(max_seq_len, d_model)
        decoder_layer = nn.TransformerDecoderLayer(
            d_model=d_model,
            nhead=nhead,
            dim_feedforward=d_model * 4,
            batch_first=True,
            norm_first=True
        )
        self.transformer_decoder = nn.TransformerDecoder(decoder_layer, num_layers=num_layers)
        self.lm_head = nn.Linear(d_model, vocab_size, bias=False)
        self.register_buffer('pos_enc', self.pos_encoding)

    def _create_positional_encoding(self, max_len, d_model):
        pe = torch.zeros(max_len, d_model)
        position = torch.arange(0, max_len, dtype=torch.float).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2).float() * (-np.log(10000.0) / d_model))
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        return pe.unsqueeze(0)

    def forward(self, x, use_cache=False, past_key_values=None):
        B, L = x.shape
        x = self.embedding(x) * (self.d_model ** 0.5)
        x = x + self.pos_enc[:, :L, :]

        tgt_mask = torch.triu(torch.full((L, L), float('-inf')), diagonal=1).to(x.device)

        if not use_cache:
            output = self.transformer_decoder(x, x, tgt_mask=tgt_mask)
            logits = self.lm_head(output)
            return logits, None
        else:
            present_key_values = []
            hidden = x

            for i, layer in enumerate(self.transformer_decoder.layers):
                attn = layer.self_attn
                # 投影 Q, K, V
                embed_dim = attn.embed_dim
                head_dim = embed_dim // attn.num_heads
                if hasattr(attn, 'in_proj_weight'):
                    qkv = F.linear(hidden, attn.in_proj_weight, attn.in_proj_bias)
                    q, k, v = qkv.chunk(3, dim=-1)
                else:
                    # 如果有分离投影（如自定义模型），需调整
                    q = F.linear(hidden, attn.q_proj.weight, attn.q_proj.bias)
                    k = F.linear(hidden, attn.k_proj.weight, attn.k_proj.bias)
                    v = F.linear(hidden, attn.v_proj.weight, attn.v_proj.bias)

                q = q.view(B, L, attn.num_heads, head_dim).transpose(1, 2)
                k = k.view(B, L, attn.num_heads, head_dim).transpose(1, 2)
                v = v.view(B, L, attn.num_heads, head_dim).transpose(1, 2)

                if past_key_values is not None:
                    past_k, past_v = past_key_values[i]
                    k = torch.cat([past_k, k], dim=2)
                    v = torch.cat([past_v, v], dim=2)

                present_key_values.append((k, v))

                attn_weights = torch.matmul(q, k.transpose(-2, -1)) / (head_dim ** 0.5)
                if k.size(2) > 1:
                    causal_mask = torch.triu(torch.full_like(attn_weights, float('-inf')), diagonal=1)
                    attn_weights += causal_mask
                attn_probs = F.softmax(attn_weights, dim=-1)
                attn_output = torch.matmul(attn_probs, v)
                attn_output = attn_output.transpose(1, 2).contiguous().view(B, L, embed_dim)

                if hasattr(attn, 'out_proj'):
                    attn_output = attn.out_proj(attn_output)
                else:
                    attn_output = F.linear(attn_output, attn.out_proj.weight, attn.out_proj.bias)

                hidden = layer.norm1(hidden + attn_output)
                ff_output = layer.linear2(F.relu(layer.linear1(hidden)))
                hidden = layer.norm2(hidden + ff_output)

            logits = self.lm_head(hidden)
            return logits, present_key_values

# ----------------------------
# 训练（简化：仅初始化模型，跳过训练以避免 OOM）
# ----------------------------
print("Initializing large model...")
model = TransformerQAModel(vocab_size, d_model, nhead, num_layers, max_seq_len).to(device)
# 注释掉训练循环，直接推理（避免显存不足）
# 实际使用时可加载预训练权重
# optimizer = optim.AdamW(model.parameters(), lr=5e-4)
# criterion = nn.CrossEntropyLoss(ignore_index=0)  # 忽略 padding

# model.train()
# for epoch in range(epochs):
#     total_loss = 0
#     for batch in tqdm(dataloader, desc=f"Epoch {epoch+1}"):
#         batch = batch.to(device)
#         input_ids = batch[:, :-1]
#         labels = batch[:, 1:]

#         optimizer.zero_grad()
#         logits, _ = model(input_ids, use_cache=False)
#         loss = criterion(logits.reshape(-1, vocab_size), labels.reshape(-1))
#         loss.backward()
#         optimizer.step()
#         total_loss += loss.item()
#     print(f"Epoch {epoch+1}, Loss: {total_loss/len(dataloader):.4f}")

# ----------------------------
# 推理函数（带 Redis 分片缓存）
# ----------------------------
def generate_answer_with_redis(model, input_tokens, max_new_tokens=200, eos_token_id=3, session_id=None):
    model.eval()
    if session_id is None:
        session_id = str(uuid.uuid4())

    input_ids = torch.tensor([input_tokens], dtype=torch.long).to(device)
    generated = input_tokens.copy()

    with torch.no_grad():
        for step in range(max_new_tokens):
            # ✅ 修复：传入 device
            past_kv = get_past_kv_from_redis(session_id, num_layers=model.num_layers, device=device)

            logits, present_kv = model(input_ids, use_cache=True, past_key_values=past_kv)

            next_token = torch.argmax(logits[:, -1, :], dim=-1).item()
            generated.append(next_token)
            if next_token == eos_token_id:
                break

            save_past_kv_to_redis(session_id, present_kv, ttl_seconds=TTL_SECONDS)

            input_ids = torch.tensor([[next_token]], dtype=torch.long).to(device)

    return generated, session_id

# ----------------------------
# 测试
# ----------------------------
if __name__ == "__main__":
    print(f"Using device: {device}")
    print("Generating long answer to test large KV cache...")

    question1 = [1, 10, 11, 12, 2]
    answer1, sid = generate_answer_with_redis(model, question1, max_new_tokens=600)
    print(f"Answer 1 length: {len(answer1)}")

    follow_up = [1, 13, 14, 2]
    full_input = answer1[:-1] + follow_up
    answer2, _ = generate_answer_with_redis(model, full_input, session_id=sid, max_new_tokens=600)
    print(f"Answer 2 length: {len(answer2)}")

    print("✅ Done. Check Redis keys with: redis-cli KEYS 'kv:*'")