make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- keystone_defconfig;
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  zImage -j8
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- UIMAGE_LOADADDR=0x8000 uImage -j8
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- keystone-k2hk-evm.dtb
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  modules
#sudo qemu-system-arm -M vexpress-a15 -m 256M -kernel arch/arm/boot/uImage -initrd rootfs
