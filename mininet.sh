sudo modprobe -r tcp_probe
sudo modprobe tcp_probe port=0 full=1 bufsize=128
sudo cat /proc/net/tcpprobe > trace.log &
TCPCAP=$!
sudo dmesg -c &&
sudo python router.py
kill $TCPCAP
