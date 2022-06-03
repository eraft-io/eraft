import redis
import random

def connect_server(host, port):
    pool = redis.ConnectionPool(host = host, port = port, decode_responses = True)
    server = redis.Redis(connection_pool = pool)
    return server

server = connect_server('localhost', 6379)

server.flushall()

for i in range(100000):
    op = random.randint(0, 5)
    small_signed_count = random.randint(-4, 5)
    if random.randint(0, 1) == 0:
        ele = random.randint(0, 999)
    else:
        ele = 'x' * random.randint(0, 99) + str(random.randint(0, 999))
    try:
        if op == 0:
            server.lpush('key', ele)
        elif op == 1:
            server.rpush('key', ele)
        elif op == 2:
            server.lpop('key')
        elif op == 3:
            server.rpop('key')
        elif op == 4:
            server.lset('key', small_signed_count, ele)
        elif op == 5:
            otherele = random.randint(0, 999)
            if random.randint(0, 1) == 0:
                where = 'before'
            else:
                where = 'after'
            server.linsert('key', where, otherele, ele)
    except redis.exceptions.ResponseError:
        pass

