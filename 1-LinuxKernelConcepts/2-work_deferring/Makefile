
# print the value of PROGS
ifneq ($(KERNELRELEASE),)
obj-m := $(PROGS)
else
KDIR ?= /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/bb/linux
# Source files
SRCS := $(wildcard *.c)
# Module object files
PROGS := $(SRCS:.c=.o)
MODS := $(SRCS:.c=.ko)
EXTRA_CFLAGS += -DDEBUG
all:
	$(MAKE) -C $(KDIR) M=$(PWD) PROGS="$(PROGS)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" modules 
	sudo cp $(MODS) /srv/nfs4/nfsroot_bb/root
endif

clean:
	sudo rm /srv/nfs4/nfsroot_bb/root/*.ko
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Target to print the values of SRCS and PROGS
print-vars:
	@echo "SRCS = $(SRCS)"
	@echo "PROGS = $(PROGS)"
