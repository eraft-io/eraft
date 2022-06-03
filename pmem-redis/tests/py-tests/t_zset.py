import os
import time
import redis

print('============starting redis-server==================')

port = 1228
os.system('kill -9 $(ps -ax | grep "redis-server \*:%s" | awk \'{print $1}\')' % port)
os.system('../../src/redis-server --nvm-maxcapacity 1 --nvm-dir /mnt/pmem0 --nvm-threshold 64' \
          ' --zset-max-ziplist-entries 3 --zset-max-ziplist-value 128 --port %s &' % port)
time.sleep(2)

pool = redis.ConnectionPool(host = 'localhost', port = port, decode_responses = True)
server = redis.Redis(connection_pool = pool)

print('===========staring tests===================')

def assert_nvm_count(server, count):
    info = server.info('memory')
    assert(info['allocated_nvm_count'] == count)

def assert_encoding(server, key, enc):
    assert(server.object('encoding', key) == enc)

def test_zadd(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    server.zadd('A', ele1, 1.0)
    assert_nvm_count(server, 1)
    ele2 = 'y' * 100
    server.zadd('A', ele2, 2.0)
    assert_nvm_count(server, 2)
    assert_encoding(server, 'A', 'ziplist')
    ele3 = 'hello'
    server.zadd('A', ele3, 3.0)
    assert_nvm_count(server, 2)
    assert_encoding(server, 'A', 'ziplist')

def test_zrem(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    server.zadd('A', ele1, 1.0)
    assert_nvm_count(server, 1)
    ele2 = 'y' * 100
    server.zadd('A', ele2, 2.0)
    assert_nvm_count(server, 2)
    server.zrem('A', ele2)
    assert_nvm_count(server, 1)

def test_zset_convert_from_ziplist_to_skiplist(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    ele2 = 'y' * 100
    server.zadd('A', ele1, 1.0)
    server.zadd('A', ele2, 2.0)
    assert_nvm_count(server, 2)
    assert_encoding(server, 'A', 'ziplist')
    server.zadd('A', 'a', 3.0)
    server.zadd('A', 'b', 4.0)
    assert_encoding(server, 'A', 'skiplist')
    assert_nvm_count(server, 2)

def test_zunionstore(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    ele2 = 'y' * 100
    server.zadd('A', ele1, 1.0)
    server.zadd('A', ele2, 2.0)
    assert_encoding(server, 'A', 'ziplist')
    assert_nvm_count(server, 2)
    server.zadd('B', 'c', 7.0)
    server.zadd('B', 'd', 8.0)
    assert_encoding(server, 'B', 'ziplist')
    server.zunionstore('U', ('A', 'B'))
    assert_encoding(server, 'U', 'skiplist')
    assert_nvm_count(server, 4)
    server.delete('A')
    assert_nvm_count(server, 2)
    server.delete('U')
    assert_nvm_count(server, 0)

def test_zinterstore(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    ele2 = 'y' * 100
    server.zadd('A', ele1, 1.0)
    server.zadd('A', ele2, 2.0)
    assert_encoding(server, 'A', 'ziplist')
    assert_nvm_count(server, 2)
    server.zadd('A', 'c', 3.0)
    server.zadd('A', 'd', 4.0)
    server.zadd('A', 'e', 5.0)
    assert_encoding(server, 'A', 'skiplist')
    server.zadd('B', ele1, 1.0)
    server.zadd('B', 'c', 7.0)
    server.zadd('B', 'd', 4.0)
    server.zadd('B', 'e', 5.0)
    assert_nvm_count(server, 3)
    assert_encoding(server, 'B', 'skiplist')
    server.zinterstore('I', ('A', 'B'))
    assert_encoding(server, 'I', 'skiplist')
    assert_nvm_count(server, 4)
    server.delete('I')
    assert_nvm_count(server, 3)

def test_zset_convert_from_skiplist_to_ziplist(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 114
    ele2 = 'y' * 100
    server.zadd('A', ele1, 1.0)
    server.zadd('A', ele2, 2.0)
    assert_encoding(server, 'A', 'ziplist')
    assert_nvm_count(server, 2)
    server.zadd('A', 'c', 3.0)
    server.zadd('A', 'd', 4.0)
    server.zadd('A', 'e', 5.0)
    assert_encoding(server, 'A', 'skiplist')
    server.zadd('B', ele1, 1.0)
    server.zadd('B', 'c', 7.0)
    server.zadd('B', 'f', 4.0)
    server.zadd('B', 'h', 5.0)
    assert_nvm_count(server, 3)
    assert_encoding(server, 'B', 'skiplist')
    server.zinterstore('I', ('A', 'B'))
    assert_nvm_count(server, 4)
    assert_encoding(server, 'I', 'ziplist')

print('test zadd')
test_zadd(server)

print('test zrem')
test_zrem(server)

print('test zset convert from ziplist to skiplist')
test_zset_convert_from_ziplist_to_skiplist(server)

print('test zunionstore')
test_zunionstore(server)

print('test zinterstore')
test_zinterstore(server)

print('test zset convert from skiplist to ziplist')
test_zset_convert_from_skiplist_to_ziplist(server)
