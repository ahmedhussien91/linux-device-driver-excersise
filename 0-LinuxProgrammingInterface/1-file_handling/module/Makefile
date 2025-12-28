
# print the value of PROGS
ifneq ($(KERNELRELEASE),)
obj-m := $(PROGS)
else
KDIR ?= /home/dell/Desktop/Linux_course/Linux-yocto-Excersises/linux/code/bb/linux
DEPLOY_DIR ?= /srv/nfs4/bb_busybox
DEPLOY_METHOD ?= copy
# Source files
SRCS := $(wildcard *.c)
# Module object files
PROGS := $(SRCS:.c=.o)
MODS := $(SRCS:.c=.ko)
EXTRA_CFLAGS += -DDEBUG
all:
	$(MAKE) -C $(KDIR) M=$(PWD) PROGS="$(PROGS)" EXTRA_CFLAGS="$(EXTRA_CFLAGS)" modules
	@if [ "$(DEPLOY_METHOD)" = "scp" ]; then \
		scp $(MODS) $(DEPLOY_DIR); \
	else \
		sudo cp $(MODS) $(DEPLOY_DIR); \
	fi
endif

clean:
	@if [ "$(DEPLOY_METHOD)" = "scp" ]; then \
		ssh $(shell echo $(DEPLOY_DIR) | cut -d: -f1) "rm -f $(shell echo $(DEPLOY_DIR) | cut -d: -f2)*.ko"; \
	else \
		sudo rm -f $(DEPLOY_DIR)/*.ko; \
	fi
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Target to print the values of SRCS and PROGS
print-vars:
	@echo "SRCS = $(SRCS)"
	@echo "PROGS = $(PROGS)"
