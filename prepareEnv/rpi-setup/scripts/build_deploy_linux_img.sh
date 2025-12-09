
cd /opt/yocto/ycoto-excersise/
./bitbake_bb_rpi -h rpi -d sysv -s

devtool modify virtual/kernel
devtool build linux-raspberrypi
devtool deploy-target linux-raspberrypi root@192.168.1.101

ssh root@192.168.1.101 reboot