#!/bin/bash

export REDIS_PATH=/root/redis-4.0.0-volatile
export LD_LIBRARY_PATH=${REDIS_PATH}/deps/memkind/.libs/:${REDIS_PATH}/deps/pmdk/src/nondebug/

export REDIS_NUM=28
export BIND_SOCKET=1
export REQ_NUM=1000000
export DATA_SIZE=1024

##export Workloads=(set get lpush lpop hset hrem sadd spop zadd zrem)
export Workloads=(set get lpush lpop hset hdel sadd spop zadd zrem) #remove lpush for ddr
declare -A Empty_Workloads
Empty_Workloads=([get]="set" [lpop]="lpush" [hdel]="hset" [spop]="sadd" [zrem]="zadd")
#export Workloads=(zadd)

HOST="192.168.7.28"
slavehost="192.168.7.29"

function_log=${REDIS_PATH}/log/check_Data.log

rm -f ${REDIS_PATH}/log/*.csv -f > /dev/null   ##delete log/*.csv
rm -f ${function_log} > /dev/null
#-----------------collect data--------------------------------------------------------------
Collect_Date()  ###generate ${workload}.csv in ${REDIS_PATH}/log/
{
    ##ex. cd /root/redis-4.0.0-volatile/log/set
    cd ${REDIS_PATH}/log/$1 >/dev/null
    #export max_latency_0=0
    export sum_latency_99=0
    export avg_latency_99=0
    export instance_number=$2
    export sum_tps=0
    echo "$1,,,," >> ../$1.csv
    echo "latency_0,tps,,," >> ../$1.csv
    for i in `seq 1 $2`
    do
        latency_0=`cat $2_${i}.log|grep -x '99.*'|head -n 1|awk '{print $3}'`
        tps=`cat $2_${i}.log|grep 'requests per second'|awk '{print $1}'`
        latency_99=`awk 'BEGIN{printf "%.2f\n",'$latency_0'/'100'}'`
        if [ "$latency_0" == "" ]||[ "$tps" == "" ];then echo -e "\e[31m$1 $2_${i} is empty\e[0m";exit 1;fi
        echo "$latency_99,$tps,,," >> ../$1.csv
        #[ `awk -v v1=$latency_99 -v v2=$max_latency_0 'BEGIN{print(v1>v2)?"0":"1"}'` == "0" ] && export max_latency_0=$latency_99
        sum_latency_99=`awk -v v1=$sum_latency_99 -v v2=$latency_99 'BEGIN {printf "%0.2f",(v1+v2)}'`
        sum_tps=`awk -v v1=$sum_tps -v v2=$tps 'BEGIN {printf "%0.2f",(v1+v2)}'`
    done
    avg_latency_99=`awk 'BEGIN{printf "%.2f\n",'$sum_latency_99'/'$instance_number'}'`
    #echo "$max_latency_0,$sum_tps,,," >> ../$1.csv
    echo "avg_latency_99,sum_tps,,," >> ../$1.csv
    echo "$avg_latency_99,$sum_tps,,," >> ../$1.csv
    sleep 3
    cd - >/dev/null
}

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
#for workload in lpush #zadd set
for workload in `echo ${Workloads[*]}`
do
    #[ $workload == "hrem" ] && continue   ###hrem is no data 
    if [ $workload == "set" ] || [ $workload == "get" ];then export KEY_RANGE=12000000;export Field_Range="";else export KEY_RANGE=1000;Field_Range="-f 1024";fi
    [ ! -d ${REDIS_PATH}/log/${workload} ] && mkdir -p ${REDIS_PATH}/log/${workload}
    #--------------------------Flushall the data------------------------------------------------------    
    for (( instances=1; instances <= $REDIS_NUM; instances++ ))
    do
        port=$((9000 + ${instances}))
        core_config=$((${LOCAL_THREAD}+${instances}-1)),$((${REMOTE_THREAD} + ${instances}-1))
        echo -e  "\e[33mnumactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-cli -h $HOST -p ${port} flushall >/dev/null &\e[0m"
        numactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-cli -h $HOST -p ${port} flushall >/dev/null &
    done
    sleep 20
    #--------------------------For get operation, fill the dataset firstly------------------------------------------------------
    for key in $(echo ${!Empty_Workloads[*]})
    do
        if [ $workload == $key ];then
            echo -e "\e[33m For $key workloads, need to do the ${Empty_Workloads[$key]} operation first\e[0m"
            for (( instances=1; instances <= $REDIS_NUM; instances++ ))
            do
                port=$((9000 + ${instances}))
                core_config=$((${LOCAL_THREAD}+${instances}-1)),$((${REMOTE_THREAD} + ${instances}-1))
                echo -e "\e[33m numactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-benchmark-seq -h $HOST -p ${port} --seq ${REQ_NUM} -n ${REQ_NUM} -t ${Empty_Workloads[$key]} >/dev/null & \e[0m"
                numactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-benchmark-seq -h $HOST -p ${port} --seq ${REQ_NUM} -n ${REQ_NUM} -t ${Empty_Workloads[$key]} >/dev/null &
            done
            #--------------------------Waiting benchmark finish------------------------------------------------------
            while [ $(ps -ef | grep -c redis-benchmark) -gt 1 ];do
                echo -e "\e[33m Waiting $(($(ps -ef | grep -c redis-benchmark)-1)) benchmark-seq finish \e[0m"
                sleep 5
            done
            echo -e "\e[33m Waiting 200 seconds for benchmark-seq commands replicate to slave \e[0m"
            sleep 200
        fi
    done
    #--------------------------start testing------------------------------------------------------
    for (( instances=1; instances <= $REDIS_NUM; instances++ ))
    do
        port=$((9000 + ${instances}))
        core_config=$((${LOCAL_THREAD}+${instances}-1)),$((${REMOTE_THREAD} + ${instances}-1))
        echo -e  "\e[33m$workload starting redis client $instances\e[0m"
        echo -e "\e[33m[$workload]=>numactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-benchmark -h ${HOST} -p ${port} -r ${KEY_RANGE} -n ${REQ_NUM} -d $DATA_SIZE -t ${workload} $Field_Range > ${REDIS_PATH}/log/${REDIS_NUM}_$instances.log  &\e[0m"
        numactl -m ${BIND_SOCKET} taskset -c $core_config ${REDIS_PATH}/src/redis-benchmark -h $HOST -p ${port} -r ${KEY_RANGE} -n ${REQ_NUM} -d $DATA_SIZE -t ${workload} ${Field_Range} > ${REDIS_PATH}/log/${workload}/${REDIS_NUM}_$instances.log  &
    done
    #sleep 5
    #--------------------------Waiting benchmark finish------------------------------------------------------
    while [ $(ps -ef | grep -c redis-benchmark) -gt 1 ];do
        echo -e "\e[33m Waiting $(($(ps -ef | grep -c redis-benchmark)-1)) benchmark finish \e[0m"
        sleep 5
    done
    Collect_Date $workload $REDIS_NUM
    #-------------------ensure the replication is done------------------------------------------------
    echo -e "\e[33m Waiting 200 second to check the data consistent\e[0m"
    sleep 200
    #-----------------check master slave data's consistent--------------------------------------------
    for (( instances=1; instances <= $REDIS_NUM; instances++ ))
    do
        sleep 2
        port=$((9000 + ${instances}))
        echo -e "\e[33m =====start comparing key count between master and slave...=====\n\e[0m"
        ${REDIS_PATH}/src/redis-cli -h $HOST -p $port info keyspace > masterkey.txt
        ${REDIS_PATH}/src/redis-cli -h $slavehost -p $port info keyspace > slavekey.txt
        diff masterkey.txt slavekey.txt
        ret=$?
        # echo $ret
        if [[ $ret -eq 0 ]]; then
            echo "${workload}/${REDIS_NUM}_$instances Passed. Master and slave key count are the same." 2>&1 | tee -a ${function_log}
        else
            echo "${workload}/${REDIS_NUM}_$instances Failed. Master and slave key count are the different." 2>&1 | tee -a ${function_log}
        fi
        echo -e "\e[33m =====start comparing DB digetst code between master and slave...=====\n\e[0m"
        ${REDIS_PATH}/src/redis-cli -h $HOST -p $port debug digest > masterkey.txt
        ${REDIS_PATH}/src/redis-cli -h $slavehost -p $port debug digest > slavekey.txt
        diff masterkey.txt slavekey.txt
        ret=$?
        if [[ $ret -eq 0 ]]; then
            echo "${workload}/${REDIS_NUM}_$instances Passed. Master and slave digest id are the same." 2>&1 | tee -a ${function_log}
        else
            echo "${workload}/${REDIS_NUM}_$instances Failed. Master and slave digett id are the different." 2>&1 | tee -a ${function_log}
        fi
        rm -rf masterkey.txt slavekey.txt
    done 
done

#generate redis.csv and clean some temp file
cd ${REDIS_PATH}/log >/dev/null
paste *.csv > redis.csv
ls *.csv|grep -v redis|xargs rm -f > /dev/null
cd - >/dev/null

ssh $slavehost pkill redis
pkill redis
