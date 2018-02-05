sudo chmod a+rwx -R /sys/kernel/debug/
cd /sys/kernel/debug/tracing
echo 1 > events/tcp/tcp_probe/enable
#echo "dport == 0"  > events/tcp/tcp_probe/filter
cat trace_pipe > /home/tamim/net-next/trace.data &
pid=$!
echo $pid
