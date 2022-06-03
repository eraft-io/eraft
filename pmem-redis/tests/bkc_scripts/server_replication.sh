#!/bin/bash

pkill redis-server
export REDIS_PATH=/root/redis-4.0.0-volatile
export LD_LIBRARY_PATH=${REDIS_PATH}/deps/memkind/.libs/:${REDIS_PATH}/deps/pmdk/src/nondebug/
export REDIS_NUM=24
export BIND_SOCKET=0

MASTER_HOST="192.168.16.1"
SLAVE_HOST="192.168.16.100"
rm -rf dump.rdb


#---------------------------check master cpu configuration------------------------------------------
THREADS=`lscpu |grep "Thread(s) per core:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
CORES=`lscpu |grep "Core(s) per socket:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
if [ "$THREADS" == "1" ]; then
  echo "hyperthread is disabled, please enable it before doing the test."
  exit 1
fi

LOCAL_THREAD=`lscpu |grep "NUMA node${BIND_SOCKET} CPU(s):"| awk '{print $(NF)}'|awk -F ',' '{print $1}'|awk -F '-' '{print $1}'`
if  [ $REDIS_NUM -gt $CORES ]; then
    echo "You're running too many Redis servers! Each Redis server need 2 CPU threads."
    exit 1
fi
REMOTE_THREAD=$(($LOCAL_THREAD + $CORES*2))

#---------------------------check slave cpu configuration------------------------------------------
SLAVE_THREADS=`ssh $SLAVE_HOST lscpu |grep "Thread(s) per core:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
SLAVE_CORES=`ssh $SLAVE_HOST lscpu |grep "Core(s) per socket:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
if [ "$SLAVE_THREADS" == "1" ]; then
  echo "hyperthread is disabled, please enable it before doing the test."
  exit 1
fi

SLAVE_LOCAL_THREAD=`ssh $SLAVE_HOST lscpu |grep "NUMA node${BIND_SOCKET} CPU(s):"| awk '{print $(NF)}'|awk -F ',' '{print $1}'|awk -F '-' '{print $1}'`
if  [ $REDIS_NUM -gt $SLAVE_CORES ]; then
    echo "You're running too many Redis servers! Each Redis server need 2 CPU threads. Slave machine does not have enough core."
    exit 1
fi
SLAVE_REMOTE_THREAD=$(($SLAVE_LOCAL_THREAD + $SLAVE_CORES*2))

#--------------------------start master servers------------------------------------------------------
for (( instances=1; instances <= $REDIS_NUM; instances++ ))
do
    port=$((9000 + ${instances}))
    core_config=$((${LOCAL_THREAD}+${instances}-1)),$((${REMOTE_THREAD} + ${instances}-1))

    echo -e "\e[33mstarting redis server $instances\e[0m"
    echo -e "\e[33mnumactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/redis-server --appendonly no --bind ${MASTER_HOST} --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem0/ --nvm-threshold 64\e[0m"
    numactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/src/redis-server --appendonly no --bind ${MASTER_HOST} --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem0/ --nvm-threshold 64 &

done
#--------------------------start slave replication servers------------------------------------------------------
for (( instances=1; instances <= $REDIS_NUM; instances++ ))
do
    port=$((9000 + ${instances}))
    core_config=$((${SLAVE_LOCAL_THREAD}+${instances}-1)),$((${SLAVE_REMOTE_THREAD} + ${instances}-1))

    echo -e "\e[33mstarting redis server $instances\e[0m"
    echo -e "\e[33mnumactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/redis-server --appendonly no --bind {SLAVE_HOST} --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem1/ --nvm-threshold 64 --slaveof ${MASTER_HOST} ${port}\e[0m"
    ssh $SLAVE_HOST numactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/src/redis-server --appendonly no --bind ${SLAVE_HOST} --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem1/ --nvm-threshold 64 --slaveof ${MASTER_HOST} ${port} &

done
