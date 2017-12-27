sudo -i &&
#There should be a space between 7 and >
echo 7 > /proc/sys/kernel/printk &&
cat /proc/sys/kernel/printk &&
exit

