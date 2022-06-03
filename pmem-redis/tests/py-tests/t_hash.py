import os
import time
import redis

print('============starting redis-server==================')

port = 1228
os.system('kill -9 $(ps -ax | grep "redis-server \*:%s" | awk \'{print $1}\')' % port)
os.system('../../src/redis-server --nvm-maxcapacity 1 --nvm-dir /mnt/pmem0 --nvm-threshold 64' \
          ' --hash-max-ziplist-entries 3 --hash-max-ziplist-value 128 --port %s &' % port)
time.sleep(2)

pool = redis.ConnectionPool(host = 'localhost', port = port, decode_responses = True)
server = redis.Redis(connection_pool = pool)

print('===========staring tests===================')

def assert_nvm_count(server, count):
    info = server.info('memory')
    assert(info['allocated_nvm_count'] == count)

def assert_encoding(server, key, enc):
    assert(server.object('encoding', key) == enc)

def test_hset(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    server.hset('A', 'f1', ele1)
    info = server.info('memory')
    print(info['allocated_nvm_count'])
    assert_nvm_count(server, 1)
    ele2 = 'y' * 100
    server.hset('A', 'f2', ele2)
    assert_nvm_count(server, 2)
    assert_encoding(server, 'A', 'ziplist')
    ele3 = 'hello'
    server.hset('A', 'f1', ele3)
    assert_nvm_count(server, 1)
    assert_encoding(server, 'A', 'ziplist')

def test_hdel(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    server.hset('A', 'f1', ele1)
    assert_nvm_count(server, 1)
    ele2 = 'y' * 100
    server.hset('A', 'f2', ele2)
    assert_nvm_count(server, 2)
    server.hdel('A', 'f1')
    assert_nvm_count(server, 1)
    server.delete('A')
    assert_nvm_count(server, 0)

def test_hash_convert_from_ziplist_to_hashtable(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    server.hset('A', 'f1', ele1)
    assert_nvm_count(server, 1)
    ele2 = 'y' * 100
    server.hset('A', 'f2', ele2)
    assert_nvm_count(server, 2)
    assert_encoding(server, 'A', 'ziplist')
    server.hset('A', 'f3', 'a')
    server.hset('A', 'f4', 'b')
    assert_encoding(server, 'A', 'hashtable')
    assert_nvm_count(server, 2)
    server.delete('A')
    assert_nvm_count(server, 0)

print('test hset')
test_hset(server)

print('test hdel')
test_hdel(server)

print('test hash convert from ziplist to hashtable')
test_hash_convert_from_ziplist_to_hashtable(server)
