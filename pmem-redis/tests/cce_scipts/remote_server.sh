#!/bin/bash
socket=$1
core_bind=$2
redis_num=$3
aep_size=$4
aep_device=$5
aof_enable=$6
aof_path=$7
redis_path=$8
server_ip=$9
zeroout=${10}

port_start=$((9000 + $((socket*200))))
#ret=`ps aux | grep -v grep | grep "emon -i /home/dennis/emon/skx-2s-events.txt"|wc -l`
#if [ $ret == 0 ]; then
#/home/dennis/emon/sepdk/src/rmmod-sep
#/home/dennis/emon/sepdk/src/insmod-sep
#echo "/home/dennis/emon/bin64/emon -i /home/dennis/emon/skx-2s-events.txt >> $redis_path/${redis_num}_${aep_size}.emon &"
#/home/dennis/emon/bin64/emon -i /home/dennis/emon/skx-2s-events.txt >> $redis_path/${redis_num}_${aep_size}.emon &
#fi

if [ $aep_device == "no" ]; then
  echo "aep-device is not configured, continue to run without aep device."
  aep_device=
else
  if [ $zeroout = 1 ]; then 
   echo "zeroout is 1"
   #rm $aep_device/* -rf
   #$redis_path/creat_aep_file.sh $aep_size $redis_num $port_start $aep_device 
   #$redis_path/zero-out.sh $aep_size $aep_device
  else 
   echo "zeroout is 0"
   rm $aep_device/* -rf
  fi
fi

#---------------------------check cpu configuration------------------------------------------
#if [ $core_bind == 1 ]; then
threads=`lscpu |grep -i "thread(s) per core:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
cores=`lscpu |grep -i "core(s) per socket:"|awk -F ":" '{print $2'}|tr -d '[:space:]'`
if [ "$threads" == "1" ]; then
  echo "hyperthread is disabled, please enable it before doing the test."
  exit 1
fi
#fi

#--------------------------clean up old files-------------------------------------------------
mkdir -p ${aof_path}
rm ${aof_path}/*.aof -f
#--------------------------start servers------------------------------------------------------
local_thread=`lscpu |grep "NUMA node${socket} CPU(s):"|awk '{print $(nf)}'|awk -F ',' '{print $1}'|awk -F '-' '{print $1}'|awk -F ':' '{print $2}'`
remote_thread=$(($local_thread + $cores*2))
echo "redis_num",$redis_num, "core", $cores, "socket number",$socketnum, "local_thread", ${local_thread}

if [ $core_bind == 1 ]; then
if [ $redis_num -gt $cores ]; then
    echo "you're running too many redis servers! each redis server need 2 cpu threads."
    exit 1
fi
fi

for (( instances=1; instances <= $redis_num; instances++ ))
do
    port=$((port_start+instances))
    #aep_start=$((${aep_size} * (${instances}-1)))
    aof_name=${instances}.aof
    if [ $core_bind == 1 ];then
    cores_bind=$((${local_thread}+${instances}-1)),$((${remote_thread} + ${instances}-1))
    sockets_config="taskset -c $cores_bind"
    else
    cores_bind="${local_thread}-$((${local_thread}+${cores}-1)),${remote_thread}-$((${remote_thread} + ${cores}-1))"
    sockets_config="taskset -c $cores_bind"
    echo
    fi
    #NUMA="numactl -N $socket -l $sockets_config" 
    NUMA="numactl -m $socket $sockets_config"

    if [ $aof_enable = "disable" ]; then
        if [ ! $aep_device ]; then
	  echo " $NUMA $redis_path/redis-server --bind $server_ip --appendonly no --port ${port} >& /dev/null &"
      $NUMA $redis_path/redis-server --bind $server_ip --appendonly no --port ${port} >& /dev/null &
	else
      echo "$NUMA  $redis_path/redis-server --bind $server_ip --appendonly no --port ${port} --nvm-maxcapacity ${aep_size} --nvm-dir ${aep_device} --nvm-threshold 64 >& /dev/null &"
        $NUMA  $redis_path/redis-server --bind $server_ip --appendonly no --port ${port} --nvm-maxcapacity ${aep_size} --nvm-dir ${aep_device} --nvm-threshold 64 >& /dev/null &
        fi
    elif [ $aof_enable = "enable" ]; then
        if [ ! $aep_device ]; then
        echo " $NUMA $redis_path/redis-server --bind $server_ip --appendonly yes --appendfsync always --appendfilename     ${instances}.aof --dir ${aof_path} --port ${port} >& /dev/null &"
	$NUMA  $redis_path/redis-server --bind $server_ip --appendonly yes --appendfsync always --appendfilename     ${instances}.aof --dir ${aof_path} --port ${port} >& /dev/null &
        else
        echo "$NUMA  $redis_path/redis-server --bind $server_ip --appendonly yes --appendfsync always --appendfilename ${instances}.aof  --port ${port} --pmdir ${aep_device} ${aep_size}g  >& /dev/null &"
        $NUMA  $redis_path/redis-server --bind $server_ip --appendonly yes --appendfsync always --appendfilename ${instances}.aof  --port ${port} --pmdir ${aep_device} ${aep_size}g  >& /dev/null &
        fi
    fi
done
