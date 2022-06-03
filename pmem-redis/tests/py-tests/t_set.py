import os
import time
import redis

print('============starting redis-server==================')

port = 1228
os.system('kill -9 $(ps -ax | grep "redis-server \*:%s" | awk \'{print $1}\')' % port)
os.system('../../src/redis-server --nvm-maxcapacity 1 --nvm-dir /mnt/pmem0 --nvm-threshold 64 --port %s &' % port)
time.sleep(2)

pool = redis.ConnectionPool(host = 'localhost', port = port, decode_responses = True)
server = redis.Redis(connection_pool = pool)

print('===========staring tests===================')

def assert_nvm_count(server, count):
    info = server.info('memory')
    assert(info['allocated_nvm_count'] == count)

def test_smove(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 128
    ele2 = 'y' * 128
    server.sadd('A', ele1, ele2)
    assert_nvm_count(server, 2)
    server.smove('A', 'B', ele1)
    assert_nvm_count(server, 2)
    server.delete('A')
    assert_nvm_count(server, 1)

def test_sinterstore(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 128
    ele2 = 'y' * 128
    server.sadd('A', ele1, ele2)
    assert_nvm_count(server, 2)
    server.sadd('B', ele2)
    assert_nvm_count(server, 3)
    server.sinterstore('I', 'A', 'B')
    assert_nvm_count(server, 4)
    server.delete('A')
    assert_nvm_count(server, 2)

def test_sunionstore(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 128
    ele2 = 'y' * 128
    server.sadd('A', ele1, ele2)
    assert_nvm_count(server, 2)
    ele3 = 'z' * 512
    ele4 = 'hello'
    server.sadd('B', ele3, ele4)
    assert_nvm_count(server, 3)
    server.sunionstore('U', 'A', 'B')
    assert_nvm_count(server, 6)

def test_sdiffstore(server):
    server.flushall()
    assert_nvm_count(server, 0)
    ele1 = 'x' * 128
    ele2 = 'y' * 128
    server.sadd('A', ele1, ele2)
    assert_nvm_count(server, 2)
    ele3 = 'hello'
    server.sadd('B', ele2, ele3)
    assert_nvm_count(server, 3)
    server.sdiffstore('D', 'A', 'B')
    assert_nvm_count(server, 4)

print('test smove')
test_smove(server)

print('test sinterstore')
test_sinterstore(server)

print('test sunionstore')
test_sunionstore(server)

print('test sdiffstore')
test_sdiffstore(server)
