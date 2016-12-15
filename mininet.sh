for pid in $(sudo lsof | grep tcpprobe | awk '{print $2}') ; do sudo kill $pid ; done &&
sudo modprobe -r tcp_probe &&
sudo dmesg -c > old.log &&
#port=0 means trace all ports. 
#full=1 => write log whenever receive an acknowledgement
#full=0 => write log only when Cwnd changes
sudo modprobe tcp_probe port=0 full=0 bufsize=128 &&
#Single & because it's a background task
sudo chmod a+rwx /proc/net/tcpprobe &&
sudo cat /proc/net/tcpprobe > trace.data &
TCPCAP=$! &&
echo $TCPCAP &&
sudo python router.py &&
sudo cat trace.data | grep -a  '172.16.101.1:8554 172.16' > h1.data &&
sudo cat trace.data | grep -a '172.16.103.1:8554 172.16' > h3.data &&
sudo cat trace.data | grep -a '172.16.105.1:8554 172.16' > h5.data
#sudo kill $TCPCAP &&
#sudo lsof | grep tcpprobe &&
#for pid in $(sudo lsof | grep tcpprobe | awk '{print $2}') ; do sudo kill $pid ; done &&
#sudo modprobe -r tcp_probe
