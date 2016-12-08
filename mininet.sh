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
cat trace.log | grep  '172.16.101.1:8554 172.16' > h1.log
cat trace.log | grep  '172.16.103.1:8554 172.16' > h3.log
cat trace.log | grep  '172.16.105.1:8554 172.16' > h5.log
