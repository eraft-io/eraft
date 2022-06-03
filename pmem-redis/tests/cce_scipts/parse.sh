#!/bin/bash
#!/bin/bash
config_number=$1
data_size=(256 512 1024 2048 4096) #(3072 4096)
cli_path="/home/dennis/redis_script/valprop"
SLA=1

if [ ${config_number} = "range_s_ad" ]; then
data_size=(512)
workset=("setget" "set2get8")
instance=(68 64 60 54 50 46 40 36 32 30 28 26 24 22 20 18 16 14 12 10 8 6 4 2)
fi

if [ ${config_number} = "range_s_ad_1" ]; then
data_size=(1024 2048)
workset=("setget" "set2get8")
instance=(68 64 60 54 50 46 40 36 32 30 28 26 24 22 20 18 16 14 12 10 8 6 4 2)
fi

if [ ${config_number} = "range_s_dram" ]; then
data_size=(512 1024 2048)
workset=("setget" "set2get8")
instance=(68 64 60 54 50 46 40 36 32 30 28 26 24 22 20 18 16 14 12 10 8 6 4 2)
fi


log=${cli_path}/log_${config_number}
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

