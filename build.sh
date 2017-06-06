#Sometimes make -j8 gets permission denied witout sudo
sudo make -j8 &&
sudo sh module.sh &&
sudo sh mininet.sh
