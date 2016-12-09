sudo dmesg -c > logs/old.log &&
#trace all ports. write log whenever receive an acknowledgement
sudo modprobe tcp_probe port=0 full=0 bufsize=128 &&
#Single & because it's a background task
sudo chmod a+rwx /proc/net/tcpprobe &&
sudo cat /proc/net/tcpprobe > ./logs/trace.log &
TCPCAP=$! &&
echo $TCPCAP &&
sudo python router.py &&
cd logs &&
sudo cat trace.log | grep  '172.16.101.1:8554 172.16' > h1.log &&
sudo cat trace.log | grep  '172.16.103.1:8554 172.16' > h3.log &&
sudo cat trace.log | grep  '172.16.105.1:8554 172.16' > h5.log &&
cd .. &&
sudo gnuplot -c tcp.plt ./logs/h1.log h1 &&
sudo gnuplot -c tcp.plt ./logs/h3.log h3 &&
sudo gnuplot -c tcp.plt ./logs/h5.log h5 &&
sudo kill $TCPCAP &&
sudo lsof | grep tcpprobe &&
sudo modprobe -r tcp_probe
