sudo chmod a+rwx -R /sys/kernel/debug/ &&
cd /sys/kernel/debug/tracing &&
cat available_tracers &&
#echo xdp_router_forward > set_ftrace_filter &&
#echo function_graph > current_tracer &&
echo function_graph > current_tracer &&
cat trace

