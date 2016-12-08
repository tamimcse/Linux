sudo modprobe -r tcp_probe &&
#trace all ports. write log whenever receive an acknowledgement
sudo modprobe tcp_probe port=0 full=1 bufsize=128 &&
sudo dmesg -c > logs/old.log &&
#Single & because it's a background task
sudo cat /proc/net/tcpprobe > ./logs/trace.log &
TCPCAP=$! &&
echo $TCPCAP &&
sudo python router.py &&
sudo kill $TCPCAP &&
cd logs &&
sudo cat trace.log | grep  '172.16.101.1:8554 172.16' > h1.log &&
sudo cat trace.log | grep  '172.16.103.1:8554 172.16' > h3.log &&
sudo cat trace.log | grep  '172.16.105.1:8554 172.16' > h5.log &&
cd .. &&
gnuplot -c tcp.plt ./logs/h1.log h1
#gnuplot -c tcp.plt ./logs/h3.log h3
#gnuplot -c tcp.plt ./logs/h5.log h5


