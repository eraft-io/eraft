size=$1
inst=$2
port_start=$3
aep_device=$4
for i in `seq $inst`;
do
port=$((port_start+i))
touch ${aep_device}/redis-port-${port}-${size}GB-AEP &
done

waitforcpdone() {
while true
do
ret=`ps aux | grep -v grep | grep "cp /mnt/pmem"|wc -l`
if [ $ret == 0 ]; then
  echo "completed! "
  break
fi
sleep 1
done
}

waitforcpdone



