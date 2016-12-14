for pid in $(sudo lsof | grep tcpprobe | awk '{print $2}') ; do sudo kill $pid ; done &&
sudo modprobe -r tcp_probe &&
sudo make modules_install 
#sudo modprobe tcp_probe 
