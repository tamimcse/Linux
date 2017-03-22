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
sudo chmod a+rwx backlog-im.data &&
sudo chmod a+rwx backlog-inigo.data &&
sudo chmod a+rwx backlog-cdg.data &&
sudo python router.py; 
is_mf=$(cat /proc/sys/net/ipv4/tcp_mf)
cong=$(cat /proc/sys/net/ipv4/tcp_congestion_control)
echo $is_mf
echo $cong
if [ $is_mf -eq '1' ]
then
    sudo cat trace.data | grep -a '172.16.101.1:8554 172.16' > h1-im.data &&
    sudo cat trace.data | grep -a '172.16.103.1:8554 172.16' > h3-im.data &&
    sudo cat trace.data | grep -a '172.16.105.1:8554 172.16' > h5-im.data &&
    sudo cat /proc/net/mf_probe > backlog-im.data
elif [ $cong = "cdg" ]
then
    sudo cat trace.data | grep -a '172.16.101.1:8554 172.16' > h1-cdg.data &&
    sudo cat trace.data | grep -a '172.16.103.1:8554 172.16' > h3-cdg.data &&
    sudo cat trace.data | grep -a '172.16.105.1:8554 172.16' > h5-cdg.data &&
    sudo cat /proc/net/mf_probe > backlog-cdg.data 
#    sudo cat /proc/net/mf_probe > backlog-tcp.data
else
    sudo cat trace.data | grep -a '172.16.101.1:8554 172.16' > h1-inigo.data &&
    sudo cat trace.data | grep -a '172.16.103.1:8554 172.16' > h3-inigo.data &&
    sudo cat trace.data | grep -a '172.16.105.1:8554 172.16' > h5-inigo.data &&
    sudo cat /proc/net/mf_probe > backlog-inigo.data 
fi

#sudo kill $TCPCAP &&
#sudo lsof | grep tcpprobe &&
#for pid in $(sudo lsof | grep tcpprobe | awk '{print $2}') ; do sudo kill $pid ; done &&
#sudo modprobe -r tcp_probe
