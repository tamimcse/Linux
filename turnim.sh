is_mf=$(cat /proc/sys/net/ipv4/tcp_mf)
echo $is_mf
if [ $is_mf -eq '1' ]
then
    sudo sysctl net.ipv4.tcp_mf=0
else
    sudo sysctl net.ipv4.tcp_mf=1
fi

