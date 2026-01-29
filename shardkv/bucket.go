package shardkv

type Bucket struct {
	KV     map[string]string
	Status BucketStatus
}

func NewShard() *Bucket {
	return &Bucket{make(map[string]string), Serving}
}

func (b *Bucket) Get(key string) (string, Err) {
	if value, ok := b.KV[key]; ok {
		return value, OK
	}
	return "", ErrNoKey
}

func (b *Bucket) Put(key, value string) Err {
	b.KV[key] = value
	return OK
}

func (b *Bucket) Append(key, value string) Err {
	b.KV[key] += value
	return OK
}

func (b *Bucket) deepCopy() map[string]string {
	nb := make(map[string]string)
	for k, v := range b.KV {
		nb[k] = v
	}
	return nb
}
