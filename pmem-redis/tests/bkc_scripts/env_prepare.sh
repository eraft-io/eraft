echo never > /sys/kernel/mm/transparent_hugepage/enabled
sysctl vm.overcommit_memory=1
sysctl -w net.core.somaxconn=65535
cpupower frequency-set -g performance
echo 3 >/proc/sys/vm/drop_caches
