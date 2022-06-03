import redis
import random

def connect_server(host, port):
    pool = redis.ConnectionPool(host = host, port = port, decode_responses = True)
    server = redis.Redis(connection_pool = pool)
    return server

def gen_random_key(prefix, key_space):
    key = "%s_%d" % (prefix, random.randint(1, key_space))
    return key

def gen_random_value(max_data_sz):
    opt = random.randint(1, 28)
    if opt == 1:
        char = '#'
    elif opt == 2:
        char = '@'
    else:
        char = chr(ord('a') + random.randint(0, 25))
    value = char * random.randint(1, max_data_sz)
    return value

def do_set(servers, key_space, max_data_sz):
    key = gen_random_key("string", key_space)
    value = gen_random_value(max_data_sz)
    for server in servers:
        server.set(key, value)
    return key

def do_sadd(servers, key_space, max_data_sz):
    key = gen_random_key("set", key_space)
    value = gen_random_value(max_data_sz)
    for server in servers:
        server.sadd(key, value)
    return key

def do_rpush(servers, key_space, max_data_sz):
    key = gen_random_key("list", key_space)
    value = gen_random_value(max_data_sz)
    for server in servers:
        server.rpush(key, value)
    return key

def do_hset(servers, key_space, max_data_sz):
    key = gen_random_key("hset", key_space)
    field = gen_random_key("field", key_space)
    value = gen_random_value(max_data_sz)
    for server in servers:
        server.hset(key, field, value)
    return key

def do_zadd(servers, key_space, max_data_sz):
    key = gen_random_key("zadd", key_space)
    value = gen_random_value(max_data_sz)
    score = random.randint(1, 1000)
    for server in servers:
        server.zadd(key, value, score)
    return key

def do_del(servers, key):
    for server in servers:
        server.delete(key)

servers = [connect_server('localhost', 1228), connect_server('localhost', 1229)]
key_space = 10000000
req_num = 1000000
max_data_sz = 1024
keys = set()

for i in range(req_num):
    opt = random.randint(0, 510)
    if opt < 100:
        key = do_set(servers, key_space, max_data_sz)
        keys.add(key)
    elif opt < 200:
        key = do_sadd(servers, key_space, max_data_sz)
        keys.add(key)
    elif opt < 300:
        key = do_rpush(servers, key_space, max_data_sz)
        keys.add(key)
    elif opt < 400:
        key = do_hset(servers, key_space, max_data_sz)
        keys.add(key)
    elif opt < 500:
        key = do_zadd(servers, key_space, max_data_sz)
        keys.add(key)
    elif len(keys) > 0:
        key = keys.pop()
        do_del(servers, key)
