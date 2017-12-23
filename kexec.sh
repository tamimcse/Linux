sudo make modules_install install
sudo make headers_install ARCH=i386 INSTALL_HDR_PATH=/usr
sudo kexec -l /boot/vmlinuz-4.14.0+ --initrd=/boot/initrd.img-4.14.0+ --reuse-cmdline
sudo kexec -e
