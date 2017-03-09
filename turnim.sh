is_mf=$(cat /proc/sys/net/ipv4/tcp_mf)
cong=$(cat /proc/sys/net/ipv4/tcp_congestion_control)
echo $is_mf
echo $cong

if [ $is_mf -eq '1' ]
then
    sudo sysctl net.ipv4.tcp_mf=0
    sudo sysctl net.ipv4.tcp_congestion_control=cdg
elif [ $cong = "cdg" ]
then
    sudo sysctl net.ipv4.tcp_congestion_control=bbr
else
    sudo sysctl net.ipv4.tcp_mf=1
    sudo sysctl net.ipv4.tcp_congestion_control=cubic
fi

