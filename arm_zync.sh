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
