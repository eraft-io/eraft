#!/bin/bash

pkill redis-server
export REDIS_PATH=/root/redis-4.0.0-volatile/
export LD_LIBRARY_PATH=${REDIS_PATH}/deps/memkind/.libs/:${REDIS_PATH}/deps/pmdk/src/nondebug/
export REDIS_NUM=$1
export BIND_SOCKET=0
export NVM_THRESHOLD=$2
# export Zset_max_ziplist_entries=(512 2000 512)
# export Zset_max_ziplist_value=(64 2000 2000)
export Zset_max_ziplist_entries=512
export Zset_max_ziplist_value=64
while getopts e:v: OPT;do
    case $OPT in
        e)
            export Zset_max_ziplist_entries=$OPTARG
            ;;
        v)
            export Zset_max_ziplist_value=$OPTARG  # print_trashed_file is a function
            ;;
    esac
done
echo -e "\e[33mZset_max_ziplist_entries is $Zset_max_ziplist_entries\e[0m"
echo -e "\e[33mZset_max_ziplist_value is $Zset_max_ziplist_value\e[0m"

if [ `ls /dev/pmem*|wc -l` -ne 2 ]; then
    echo "please check AD mode"
    exit 1
fi
if [ `mount|egrep "pmem0|pmem1"|wc -l` -ne 2 ]; then
    mount -o dax /dev/pmem0 /mnt/pmem0
    mount -o dax /dev/pmem1 /mnt/pmem1
fi
#---------------------------check cpu configuration------------------------------------------
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

#--------------------------start servers------------------------------------------------------
for (( instances=1; instances <= $REDIS_NUM; instances++ ))
do
    port=$((9000 + ${instances}))
    core_config=$((${LOCAL_THREAD}+${instances}-1)),$((${REMOTE_THREAD} + ${instances}-1))

    echo -e "\e[33mstarting redis server $instances\e[0m"
    echo -e "\e[33mnumactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/src/redis-server --appendonly no --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem${BIND_SOCKET}/ --nvm-threshold $NVM_THRESHOLD --dbfilename ${port}.dump --zset-max-ziplist-entries $Zset_max_ziplist_entries --zset-max-ziplist-value $Zset_max_ziplist_value\e[0m"
    numactl -m ${BIND_SOCKET} taskset -c $core_config  $REDIS_PATH/src/redis-server --appendonly no --port ${port} --nvm-maxcapacity 15 --nvm-dir /mnt/pmem${BIND_SOCKET}/ --nvm-threshold $NVM_THRESHOLD --dbfilename ${port}.dump --zset-max-ziplist-entries $Zset_max_ziplist_entries --zset-max-ziplist-value $Zset_max_ziplist_value &

done
