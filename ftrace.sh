#Enable CONFIG_FUNCTION_TRACER=y
sudo chmod a+rwx -R /sys/kernel/debug/ &&
cd /sys/kernel/debug/tracing &&
cat available_tracers &&
sudo echo xdp_router_forward ip_route_lookup is_xdp_forwardable > set_ftrace_filter &&
#echo function > current_tracer &&
echo function_graph > current_tracer &&
cat trace

