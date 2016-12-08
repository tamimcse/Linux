sudo rm -f trace.log &&
sudo modprobe -r tcp_probe &&
sudo modprobe tcp_probe port=0 full=1 bufsize=128 &&
#Single & because it's a background task
sudo cat /proc/net/tcpprobe > trace.log &
TCPCAP=$! &&
echo $TCPCAP &&
sudo dmesg -c &&
sudo python router.py &&
sudo kill $TCPCAP
