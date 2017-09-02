#for actual hardware
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- xilinx_zynq_defconfig;
#for Qemu
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- xilinx_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  UIMAGE_LOADADDR=0x8000 uImage -j8


#X86 emulation
#./qemu-img create -f qcow2 test.qcow2 16G
#sudo x86_64-softmmu/qemu-system-x86_64 -m 1024 -enable-kvm -drive if=virtio,file=test.qcow2,cache=none -cdrom ubuntu-16.04.2-desktop-amd64.iso
#sudo x86_64-softmmu/qemu-system-x86_64 -m 1024 -enable-kvm -drive if=virtio,file=test.qcow2,cache=none -kernel vmlinuz-4.11.0+ -initrd initrd.img-4.11.0+ -append "root=/dev/ram"  -no-reboot


#ARM emulation
#./configure --target-list=arm-softmmu,aarch64-softmmu,arm-linux-user --audio-drv-list=alsa,sdl,oss
#./qemu-img create -f raw armdisk.img 8G
#sudo arm-softmmu/qemu-system-arm -m 1024M -sd armdisk.img -M vexpress-a9 -cpu cortex-a9 -kernel vmlinuz-3.2.0-4-vexpress -initrd initrd.gz -append "root=/dev/ram"  -no-reboot
#sudo arm-softmmu/qemu-system-arm -m 256M -sd armdisk.img -M versatilepb -kernel vmlinuz-3.2.0-4-versatile  -initrd initrd.img-3.2.0-4-versatile  -append "root=/dev/ram"  -no-reboot


#MIPS emulation
#./configure --target-list=mips-softmmu,mipsel-softmmu,mips64-softmmu,mips64el-softmmu,mips-linux-user,mipsel-linux-user,mips64-linux-user,mips64el-linux-user,mipsn32-linux-user,mipsn32el-linux-user
#sudo mips-softmmu/qemu-system-mips -m 256M -sd armdisk.img -M versatilepb -kernel vmlinuz-3.2.0-4-versatile  -initrd initrd.img-3.2.0-4-versatile  -append "root=/dev/ram"  -no-reboot


#Rocker switchdev
./configure --target-list=x86_64-softmmu --extra-cflags=-DDEBUG_ROCKER  --disable-werror
make -j8
sudo x86_64-softmmu/qemu-system-x86_64 ­-device rocker,name=sw1,len-ports=4,ports[0]=dev1,ports[1]=dev2,ports[2]=dev3,ports[3]=dev4
sudo x86_64-softmmu/qemu-system-x86_64 -m 2048 -enable-kvm -drive if=virtio,file=test.qcow2,cache=none -device rocker,name=sw1,len-ports=8 -cdrom ubuntu-16.04.2-desktop-amd64.iso

sudo x86_64-softmmu/qemu-system-x86_64 -m 2048 -enable-kvm  -kernel /boot/vmlinuz-4.11.0+ -initrd /boot/initrd.img-4.11.0+ -drive if=virtio,file=test.qcow2,cache=none


./qemu-img create -f raw ubuntu.img 16G
sudo x86_64-softmmu/qemu-system-x86_64 -m 2048  -enable-kvm -hda ubuntu.img -cdrom ubuntu-16.04.2-desktop-amd64.iso -boot d
sudo x86_64-softmmu/qemu-system-x86_64 -m 2048  -enable-kvm -hda ubuntu.img -device rocker,name=sw1,len-ports=4


lscpi
ifconfig
ip link show | grep switch
watch -d -n 1 sensors

sudo ip address add 1.1.1.2/24 dev eth1
sudo ip link set dev eth1 up
#sudo ip address flush dev eth1

sudo ip addr add 10.0.2.1 dev eth2
sudo ip addr add 10.0.3.1 dev eth3
sudo ip addr add 10.0.4.1 dev eth4

ethtool-i eth1
tcpdump -ei eth1

#Create bridge
ip link add name br0 type bridge
ip link set dev br0 type bridge vlan_filtering 1
#ip link set dev br0 type bridge ageing_time 1000

#to make bridge functinal
ip link set dev br0 up

#add a netdev to the bridge (enslaving)
ip link set dev eth1 master br0
ip link set dev eth2 master br0
ip link set dev eth1 up
ip link set dev eth2 up

#To show all the bridges 
brctl show

#To unslave netdev from bridge
#ip link set dev DEV nomaster

bridge vlan show
ip -d link show dev br0
#To delete bridge 
ip link del dev br0

#To show the bridge forwarding table
bridge fdb show br br0
watch -d -n 1 bridge fdb show br br0



Create veth
------------------------------------------------------
# ip link add type veth
# ip link show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT 
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP mode DEFAULT qlen 1000
    link/ether 52:54:00:5a:d2:86 brd ff:ff:ff:ff:ff:ff
3: eth1: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT qlen 1000
    link/ether 52:54:00:d7:26:a6 brd ff:ff:ff:ff:ff:ff
23: veth0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT qlen 1000
    link/ether ee:c0:0e:d6:ae:09 brd ff:ff:ff:ff:ff:ff
24: veth1: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT qlen 1000
    link/ether 4e:e8:84:bd:01:f0 brd ff:ff:ff:ff:ff:ff
# ip addr add 10.0.0.1/24 dev veth0
# ip link set veth0 up
# ip link set veth1 up
# force sending packets out veth0
# ping 10.0.0.1



