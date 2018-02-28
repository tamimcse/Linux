sudo chmod a+rwx -R /sys/devices/system/cpu/
#disable CPU 1 and and CPU 2 (disable hyper threading)
sudo echo 0 > /sys/devices/system/cpu/cpu1/online
sudo echo 0 > /sys/devices/system/cpu/cpu2/online
#disable freqency scaling
echo 'performance' > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 'performance' > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
lscpu --extended
