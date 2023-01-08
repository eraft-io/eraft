#!/usr/bin/env bash

mkdir /eraft/build/build/data
rm -rf /eraft/build/build/data/*
cd /eraft/build/build; ./edb 1 &

sleep 1s

/eraft/build/raftcore/test/build/raft_tests

sleep 1s

/usr/bin/mysql --host 127.0.0.1 --port 12306 -u root
