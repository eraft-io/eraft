#!/bin/bash
config_number=$1
## WA to fix the issue that the enp129s0f0 can't config
ifconfig enp175s0f0 192.168.14.100
aep_s0="/mnt/pmem0/"
aep_s1="/mnt/pmem1/"
ser_ip=192.168.17.1
ser_s0=192.168.16.1
ser_s1=192.168.14.1
ser_bind_cpu=(0 1)
server_core_bind=0
ser_path="/home/dennis/fb_redis/fb/valprop/"
workset=("setget" "set2get8")
cli_bind_cpu=(0 1)
cli_core_bind=0
data_size=(512 1024 2048) #(3072 4096)
cli_path="/home/dennis/redis_script/valprop"
aof_enable="disable"
aof_dir="/mnt/nvme0n1/aof/"
int_max=2147483647
zeroout=0
set2get8_time=600
aep_cap=3
server_core_bind=0
data_size=(1024)
instance=(40)
client_core_bind=0
req_num=1000000
range=100000
SLA=1
zeroout=1
##if server_bind_cpu(0), client_bind_cpu (1), client port start=9000
##if server_bind_cpu (1), client_bind_cpu (0), client_port start=9200
#cli_port_start=9000

if [ ${config_number} = "s1_imbal_1" ]; then
ser_ip=127.0.0.1
ser_s1=127.0.0.1
ser_bind_cpu=(1)
data_size=(1024)
instance=(30)
cli_bind_cpu=(0)
cli_port_start=9200
fi

if [ ${config_number} = "s1_imbal_2" ]; then
data_size=(2048)
workset=("setget")
#ser_ip=192.168.16.1
#ser_s1=192.168.14.1
ser_bind_cpu=(0 1)
cli_bind_cpu=(0 1)
instance=(30)
fi

##ranges mean the ranges will only be 1/100 of the request number; the max key is the ranges.
##keep the range 0.01m and request number is 1m; then the key will be always hit especially for get
if [ ${config_number} = "test" ]; then
aep_cap=3
server_core_bind=0
data_size=(512)
instance=(2)
client_core_bind=0
req_num=10000
range=10000
SLA=1
zeroout=1
fi

if [ ${config_number} = "refresh_ad" ]; then
aep_cap=3
server_core_bind=0
data_size=(512 1024 2048)
instance=(68 64 60 54 50 46 40 39 36 32 30 28 26 24 22 20 18 16 14 12 10 8 6 4 2)
client_core_bind=0
req_num=3000000
range=30000
SLA=1
zeroout=1
fi

if [ ${config_number} = "refresh_1lm" ]; then
aep_cap=3
server_core_bind=0
data_size=(512 1024 2048)
instance=(68 64 60 54 50 46 40 39 32 30 28 26 24 22 20 18 16 14 12 10 8 6 4 2)
client_core_bind=0
req_num=3000000
range=30000
SLA=1
zeroout=1
aep_s0="no"
aep_s1="no"
fi

log=${cli_path}/log_${config_number}
#------------------------------- Benchmark function -------------------------------------------------------------
run_benchmark(){
socket=$1
core_bind=$2
data_size=$3
redis_num=$4
work_load=$5
redis_log=$log
redis_path=$cli_path
req_num=$6  #`echo $aep_cap*1024*1000/$data_size*1000|bc`
int_max=$7
port_start=$((9000 + $((socket*200))))

if [ $socket == 0 ];then
  hostcmd="-h $ser_s0"
  if [ $work_load == "set2get8" ]; then
    hostcmd="-s $ser_s0"
  fi
else 
  hostcmd="-h $ser_s1"
  if [ $work_load == "set2get8" ]; then
    hostcmd="-s $ser_s1"
  fi
fi

#---------------------------check cpu configuration------------------------------------------
threads=`lscpu |grep -i "thread(s) per core:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
cores=`lscpu |grep -i "core(s) per socket:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
if [ "$threads" == "1" ]; then
  echo "hyperthread is disabled, please enable it before doing the test."
  exit 1
fi
local_thread=`lscpu |grep "NUMA node${socket} CPU(s):"|awk '{print $(nf)}'|awk -F ',' '{print $1}'|awk -F '-' '{print $1}'|awk -F ':' '{print $2}'`
remote_thread=$(($local_thread + $cores*2))

if [ $core_bind == 1 ]; then
if  [ $redis_num -gt $cores ]; then
    echo "you're running too many redis benchmarks! each redis benchmark need 2 cpu threads."
    exit 1
fi
fi

for (( instances=1; instances <= $redis_num; instances++ ))
do
	port=$((${port_start}+${instances}))
    	if [ $core_bind == 1 ]; then
	cores_bind=$((${local_thread}+${instances}-1)),$((${remote_thread} + ${instances}-1))
	sockets_config="taskset -c $cores_bind"
	else
        echo ${local_thread}, $((${local_thread}+${cores}-1)),${remote_thread}, $((${remote_thread} + ${cores}-1))	
	cores_bind="${local_thread}-$((${local_thread}+${cores}-1)),${remote_thread}-$((${remote_thread} + ${cores}-1))"
	sockets_config="taskset -c $cores_bind"
	fi

        #NUMA="numactl -N $socket -l $sockets_config"
	NUMA="numactl -m $socket $sockets_config"
        
	if [ $work_load = "set" ]; then
	echo "$NUMA $redis_path/redis-benchmark -k 1 $hostcmd -p ${port} -r $int_max -n ${req_num} -t $work_load -d $data_size -P 3 -c 5 >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_${instances}.log &"
        $NUMA $redis_path/redis-benchmark -k 1 $hostcmd -p ${port} -r $int_max -n ${req_num} -t $work_load -d $data_size -P 3 -c 5 >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_${instances}.log &
        fi

        if [ $work_load = "get" ]; then
        echo "$NUMA $redis_path/redis-benchmark -k 1 $hostcmd -p ${port} -r $int_max -n ${req_num} -t get -d $data_size -P 3 -c 5 >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_${instances}.log &"
        $NUMA $redis_path/redis-benchmark -k 1 $hostcmd -p ${port} -r $int_max -n ${req_num} -t get -d $data_size -P 3 -c 5 >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_$instances.log &
        fi
    
        if [ $work_load = "set2get8" ]; then
	echo "$NUMA ${redis_path}/memtier_benchmark $hostcmd -p ${port} --ratio=1:4 -d $data_size --test-time=$set2get8_time -n ${req_num} --key-pattern=R:R --key-minimum=1 --key-maximum=$int_max --threads=1 --pipeline=64 -c 3 --hide-histogram >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_$instances.log &"
	$NUMA ${redis_path}/memtier_benchmark $hostcmd -p ${port} --ratio=1:4 -d $data_size -n ${req_num} --key-pattern=R:R --key-minimum=1 --key-maximum=$int_max --threads=1 --pipeline=64 -c 3 --hide-histogram >> ${redis_log}/${redis_num}_${data_size}_${work_load}_${socket}_$instances.log &
        fi    
done
}
#-----------------------------------------------------------------------------------------------------------------------------------------------------------------

waitbenchmarkdone(){
echo "start to run benchmark"
while true
do
ret=`ps aux | grep -v grep | grep "redis-benchmark"|wc -l`
if [ $ret == 0 ]; then
  echo "completed! please get the result from log directory."
  break
fi
sleep 1
done

while true
do
ret=`ps aux | grep -v grep | grep "memtier_benchmark"|wc -l`
if [ $ret == 0 ]; then
  echo "completed! please get the result from log directory."
  break
fi
sleep 1
done
}

## clean the log first
ssh ${ser_ip} pkill redis-server
ssh ${ser_ip} pkill emon
ssh ${ser_ip} pkill AEPWatch
ssh ${ser_ip} cpupower frequency-set -g performance
cpupower frequency-set -g performance
pkill redis-benchmark

rm $log -rf
mkdir -p $log

for k in ${data_size[@]}; 
do
for i in ${instance[@]};
do
for bset in ${workset[@]};  
do
    for j in ${ser_bind_cpu[@]};
    do    
      if [ $j == 0 ]; then
	bind_ip=$ser_s0
        aep_dev=$aep_s0
      else
        bind_ip=$ser_s1
	aep_dev=$aep_s1
      fi
      echo "ssh $ser_ip ${ser_path}/remote_server.sh ${j} $server_core_bind ${i} $aep_cap $aep_dev $aof_enable $aof_dir $ser_path $bind_ip"
      ssh $ser_ip ${ser_path}/remote_server.sh ${j} $server_core_bind ${i} $aep_cap $aep_dev $aof_enable $aof_dir $ser_path $bind_ip $zeroout
     done  
     sleep 30
    
     if [ $bset == "setget" ]; then 
     	ssh ${ser_ip} ${ser_path}/emon.sh ${ser_path}/${i}_${k}_set_${config_number}.emon 
     	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_set_${config_number}.aepwatch 1
	for j in ${cli_bind_cpu[@]};
     	do 
		run_benchmark $j $client_core_bind $k $i "set" $req_num $range
     	done
     	waitbenchmarkdone
	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_set_${config_number}.aepwatch 0
     	ssh ${ser_ip} pkill emon

     	ssh ${ser_ip} ${ser_path}/emon.sh ${ser_path}/${i}_${k}_get_${config_number}.emon 
	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_get_${config_number}.aepwatch 1
     	for j in ${cli_bind_cpu[@]};
     	do
        	run_benchmark $j $client_core_bind $k $i "get" $req_num $range
     	done
    	waitbenchmarkdone
	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_get_${config_number}.aepwatch 0
    	ssh ${ser_ip} pkill emon
     else
	ssh ${ser_ip} ${ser_path}/emon.sh ${ser_path}/${i}_${k}_${bset}_${config_number}.emon 
	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_${bset}_${config_number}.aepwatch 1
        for j in ${cli_bind_cpu[@]};
        do
                run_benchmark $j $client_core_bind $k $i ${bset} $req_num $range
        done
        waitbenchmarkdone
	ssh ${ser_ip} ${ser_path}/aepwatch.sh ${ser_path}/${i}_${k}_${bset}_${config_number}.aepwatch 0
        ssh ${ser_ip} pkill emon
     fi

    ssh ${ser_ip} pkill redis-server
    sleep 5
done
done
done

#-------------------------------parse data start--------------------------------------------
summary_csv=${cli_path}/${config_number}.csv
rm ${summary_csv} -rf
echo $summary_csv

SLA=$((SLA*100)) ##unit is 0.1ms
for i in ${instance[@]};
  do
  for j in ${data_size[@]};
  do
  for k in ${workset[@]};
  do
   if [ $k = "setget" ];then
    echo "./parse_debug.sh  $config_number $log $i $j "set" $SLA &"
    ./parse_debug.sh  $config_number $log $i $j "set" $SLA &
    ./parse_debug.sh $config_number $log $i $j "get" $SLA &
   else
    ./parse_debug.sh $config_number $log $i $j $k $SLA &
   fi
done
done
done

waitparsedone(){
while true
do
ret=`ps aux | grep -v grep | grep "parse_debug.sh"|wc -l`
if [ $ret == 0 ]; then
  echo "parse completed! "
  break
fi
sleep 1
done
}

waitparsedone
for i in ${instance[@]};
  do
  for j in ${data_size[@]};
  do
  for k in ${workset[@]};
  do
  if [ $k = "setget" ];then
   cat $log/${i}_${j}_set.csv >> $summary_csv
   cat $log/${i}_${j}_get.csv >> $summary_csv
  else
   cat $log/${i}_${j}_${k}.csv>>$summary_csv
  fi
done
done
done
#--------------------------parse data end ------------------------------------------------

