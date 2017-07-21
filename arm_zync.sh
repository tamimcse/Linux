#for actual hardware
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- xilinx_zynq_defconfig;
#for Qemu
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- xilinx_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  UIMAGE_LOADADDR=0x8000 uImage -j8

#./qemu-img create -f qcow2 test.qcow2 16G
#sudo x86_64-softmmu/qemu-system-x86_64 -m 1024 -enable-kvm -drive if=virtio,file=test.qcow2,cache=none -cdrom ubuntu-16.04.2-desktop-amd64.iso
