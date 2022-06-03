#!/bin/bash
#!/bin/bash
config_number=$1
log=$2
i=$3
j=$4
k=$5
SLA=$6

pushd $log

h=$i
insnum=$((i*2))
line="$insnum,$j,$k"
tps=0
l99=0
l999=0
l9999=0
l100=0
sla=0
lxx=0
lyy=0

summary_csv=$log/${i}_${j}_${k}.csv

sockets=(0 1)
for l in ${sockets[@]};
do
  for m in `seq ${i}`;
  do
    if [ $k = "set2get8" ]; then
   	score=`grep -r "Total" ${h}_${j}_${k}_${l}_${m}.log|awk '{print $2}'|tail -1`
    	l_100=`grep -r "Total" ${h}_${j}_${k}_${l}_${m}.log|awk '{print $5}'|tail -1` 
    	score=`echo $score|bc`
    	tps=`echo $tps+$score|bc`
    	l_100=`echo $l_100*1000|bc`
    	l100=`echo $l100+$l_100|bc`
    elif [ $k = "set" ] || [ $k = "get" ];then
    	score=`grep -r "requests per second" ${h}_${j}_${k}_${l}_${m}.log|awk '{print $1}'`
    	l_99=`grep -r '^99.' ${h}_${j}_${k}_${l}_${m}.log|head -n 1|awk '{print $3}'` 
    	l_999=`grep -r '^99.9' ${h}_${j}_${k}_${l}_${m}.log|head -n 1|awk '{print $3}'`
    	l_9999=`grep -r '^99.99' ${h}_${j}_${k}_${l}_${m}.log|head -n 1|awk '{print $3}'`
    	l_100=`grep -r '^100.' ${h}_${j}_${k}_${l}_${m}.log|head -n 1|awk '{print $3}'`
    	slap=`grep "<= 1[0-9][0-9] "  ${h}_${j}_${k}_${l}_${m}.log|head -n 1|awk -F % '{print $1}'`
    	score=`echo $score|bc`
    	tps=`echo $tps+$score|bc`
    	slap=`echo $slap|bc`
    	l_99=`echo $l_99|bc`
    	l_999=`echo $l_999|bc`
    	l_9999=`echo $l_9999|bc`
    	l_100=`echo $l_100|bc`
    	l99=`echo $l99+$l_99|bc`
    	l999=`echo $l999+$l_999|bc`
    	l9999=`echo $l9999+$l_9999|bc`
    	l100=`echo $l100+$l_100|bc`
    	sla=`echo $sla+$slap|bc`
    fi
  done
  lxx=`echo "$l99/$i"|bc`
  lyy=`echo "$l100/$i"|bc`
  line+=",$tps,$lxx,$lyy" 	
done

l99=`echo "$l99/$insnum"|bc`
l999=`echo "$l999/$insnum"|bc`
l9999=`echo "$l9999/$insnum"|bc`
l100=`echo "$l100/$insnum"|bc`
sla=`echo "$sla/$insnum"|bc`

line+=",$tps,$l99,$l999,$l9999,$l100,$sla"
line+=",,,,"
echo $line>$summary_csv
echo $line
popd

