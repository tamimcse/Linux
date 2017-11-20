#sudo sar -P ALL
#sudo sar -P 1
#sudo sar -P 2
ps -aF | grep iperf

#sudo taskset -c 1 bash -c 'while true ; do echo hello >/dev/null; done'
