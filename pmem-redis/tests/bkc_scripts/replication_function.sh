#! /usr/bin/bash
masterhost="192.168.16.1"
masterport="9000"
masterredis="/root/lina/redis-4.0-deg/redis-test/redis-4.0.0-volatile/src"


slavehost="192.168.16.100"
slaveport="9000"
slaveredis="/root/lina/redis-4.0.0-volatile/src"

nvm_maxcapacity=20
master_nvm_dir="/mnt/pmem0"
nvm_threshold="64"

slave_nvm_dir="/mnt/pmem0"

${masterredis}/redis-server --nvm-maxcapacity $nvm_maxcapacity --nvm-dir $master_nvm_dir --nvm-threshold $nvm_threshold --bind $masterhost --port $masterport &
sleep 1


ssh $slavehost $slaveredis/redis-server --nvm-maxcapacity $nvm_maxcapacity --nvm-dir $slave_nvm_dir --nvm-threshold $nvm_threshold --bind $slavehost --port $slaveport --slaveof $masterhost $masterport &
sleep 1

${masterredis}/redis-benchmark -h $masterhost -p $masterport -t set -d 128 -n 10000 -r 10000 &
${masterredis}/redis-benchmark -h $masterhost -p $masterport -t set -d 1024 -n 1000000 -r 100000


echo "=====start comparing key count between master and slave...=====\n"
${masterredis}/redis-cli -h $masterhost -p $masterport info keyspace > masterkey.txt
${masterredis}/redis-cli -h $masterhost -p $masterport info keyspace

ssh $slavehost $slaveredis/redis-cli -h $slavehost -p $slaveport info keyspace > slavekey.txt
ssh $slavehost $slaveredis/redis-cli -h $slavehost -p $slaveport info keyspace

diff masterkey.txt slavekey.txt

ret=$?

if [[ $ret -eq 0 ]]; then
    echo "Passed. Master and slave key count are the same."
else
    echo "Failed. Master and slave key count are the different."

fi


echo "=====start comparing DB digetst code between master and slave...=====\n"
${masterredis}/redis-cli -h $masterhost -p $masterport debug digest > masterkey.txt
cat masterkey.txt

ssh $slavehost $slaveredis/redis-cli -h $slavehost -p $slaveport debug digest > slavekey.txt
cat slavekey.txt

diff masterkey.txt slavekey.txt

ret=$?

if [[ $ret -eq 0 ]]; then
    echo "Passed. Master and slave digest id are the same."
else
    echo "Failed. Master and slave digett id are the different."

fi
 
rm -rf masterkey.txt slavekey.txt

ssh $slavehost pkill redis
pkill redis

