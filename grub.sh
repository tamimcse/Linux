#sh grub.sh 1 
if [ $1 -eq '1' ] 
then
    sudo sed -i 's/^.*GRUB_CMDLINE_LINUX_DEFAULT=.*$/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash crashkernel=384M-:128M nohz_full=1-3 isolcpus=1-3 rcu_nocbs=1-3 intel_idle.max_cstate=0 irqaffinity=0,1 selinux=0 audit=0 tsc=reliable"/' ../../../etc/default/grub
else
    #sh grub.sh 0
    sudo sed -i 's/^.*GRUB_CMDLINE_LINUX_DEFAULT=.*$/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash crashkernel=384M-:128M"/' ../../../etc/default/grub
fi
(cd ../../../etc/default/ && sudo update-grub)
