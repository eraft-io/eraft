#!/bin/bash
log=$1
kickoff=$2
if [ $kickoff = 1 ]; then
source /home/dennis/AEPWatch_1.1.0_linux_package/AEPWatch_1.1.0/aep_vars.sh
echo "AEPWatch 1 > $log &"
AEPWatch 1 > $log &
else
source /home/dennis/AEPWatch_1.1.0_linux_package/AEPWatch_1.1.0/aep_vars.sh
AEPWatch-stop
fi
