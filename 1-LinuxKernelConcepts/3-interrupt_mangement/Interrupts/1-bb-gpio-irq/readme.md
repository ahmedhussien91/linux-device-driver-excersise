# BeagleBone Black – GPIO Interrupt (Stage 1)

## Hardware
- Button between P9_12 (GPIO1_28) and GND

## Build
./prepareEnv/bb-setup/setup_bb.sh
make

## Load
sudo cp gpio_irq.ko ""/srv/nfs4/bb_busybox/lib/modules/"
ssh root@192.168.1.107 "modeprobe gpio_irq"
# ssh root@192.168.1.107 insmod /lib/modules/gpio_irq.ko


## Test
Press the button and run:
dmesg
cat /proc/interrupts | grep gpio
