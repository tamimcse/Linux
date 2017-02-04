sudo make modules_install install
sudo make headers_install ARCH=i386 INSTALL_HDR_PATH=/usr
sudo kexec -l /boot/vmlinuz-4.9.0-rc3+ --initrd=/boot/initrd.img-4.9.0-rc3+ --reuse-cmdline
sudo kexec -e
