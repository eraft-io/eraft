size=$1
aep_device=$2
num=$((size*1024))
for i in `ls $aep_device`
do
    dd if=/dev/zero of=${aep_device}/$i bs=1024k count=$num &
done

waitforzerooutdone() {
while true
do
ret=`ps aux | grep -v grep | grep "dd if=/dev/zero"|wc -l`
if [ $ret == 0 ]; then
  echo "completed! "
  break
fi
sleep 1
done
}

waitforzerooutdone
