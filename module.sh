mod=rocker_dev
echo $mod 
for pid in $(sudo lsof | grep $mod | awk '{print $2}') ; do sudo kill $pid ; done &&
sudo modprobe -r $mod &&
sudo make modules_install 
sudo modprobe $mod 
