make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- keystone_defconfig;
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  zImage -j8
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- keystone-k2hk-evm.dtb
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  modules
