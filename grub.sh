#sh grub.sh 1 
if [ $1 -eq '2' ] 
then
    sudo sed -i 's/^.*GRUB_CMDLINE_LINUX_DEFAULT=.*$/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash crashkernel=384M-:128M nohz_full=3 isolcpus=3 rcu_nocbs=3 intel_idle.max_cstate=0 irqaffinity=0,1,2 selinux=0 audit=0 tsc=reliable"/' ../../../etc/default/grub
elif [ $1 -eq '1' ]
then
    sudo sed -i 's/^.*GRUB_CMDLINE_LINUX_DEFAULT=.*$/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash crashkernel=384M-:128M iommu=pt intel_iommu=on default_hugepagesz=1G hugepagesz=1G hugepages=4"/' ../../../etc/default/grub
else
    #sh grub.sh 0
    sudo sed -i 's/^.*GRUB_CMDLINE_LINUX_DEFAULT=.*$/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash crashkernel=384M-:128M"/' ../../../etc/default/grub
fi
(cd ../../../etc/default/ && sudo update-grub)
