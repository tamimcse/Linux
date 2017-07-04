make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- ixp4xx_defconfig;
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  UIMAGE_LOADADDR=0x8000 uImage -j8
