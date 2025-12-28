# BeagleBone Black Network Boot Configuration

## Overview
BeagleBone Black is configured for network boot using TFTP for kernel/DTB loading and NFS for root filesystem.

## Network Configuration

### BeagleBone Black
- **IP Address**: 192.168.1.107
- **Boot Method**: TFTP + NFS
- **Kernel**: `zImage_native_bb` (via TFTP)
- **Device Tree**: `am335x-boneblack.dtb` (via TFTP)
- **Root Filesystem**: `/srv/nfs4/bb_busybox` (via NFS)

### Host Server
- **IP Address**: 192.168.1.11
- **TFTP Directory**: `/srv/tftp`
- **NFS Export**: `/srv/nfs4/bb_busybox`

## U-Boot Configuration

### Boot Command
```bash
bootcmd=setenv serverip 192.168.1.11; setenv ipaddr 192.168.1.107; tftpboot 88000000 am335x-boneblack.dtb; tftpboot 0x82000000 zImage_native_bb; bootz 0x82000000 - 88000000;
```

### Manual U-Boot Setup
Connect to U-Boot console and run:

```bash
# Set network configuration
setenv serverip 192.168.1.11
setenv ipaddr 192.168.1.107

# Set boot arguments for NFS root
setenv bootargs console=ttyS0,115200n8 root=/dev/nfs nfsroot=192.168.1.11:/srv/nfs4/bb_busybox,vers=4,proto=tcp rw ip=192.168.1.107:192.168.1.11:192.168.1.1:255.255.255.0::eth0:off

# Set boot command
setenv bootcmd 'setenv serverip 192.168.1.11; setenv ipaddr 192.168.1.107; tftpboot 88000000 am335x-boneblack.dtb; tftpboot 0x82000000 zImage_native_bb; bootz 0x82000000 - 88000000'

# Save environment
saveenv

# Boot
boot
```

## Host Server Setup

### 1. TFTP Server Configuration

Install and configure TFTP server:
```bash
sudo apt-get install tftpd-hpa
sudo systemctl enable tftpd-hpa
sudo systemctl start tftpd-hpa
```

TFTP configuration (`/etc/default/tftpd-hpa`):
```bash
TFTP_USERNAME="tftp"
TFTP_DIRECTORY="/srv/tftp"
TFTP_ADDRESS=":69"
TFTP_OPTIONS="--secure"
```

Create TFTP directory:
```bash
sudo mkdir -p /srv/tftp
sudo chown tftp:tftp /srv/tftp
sudo chmod 755 /srv/tftp
```

### 2. NFS Server Configuration

Install NFS server:
```bash
sudo apt-get install nfs-kernel-server
```

Configure NFS exports (`/etc/exports`):
```bash
/srv/nfs4/bb_busybox 192.168.1.107(rw,sync,no_subtree_check,no_root_squash)
```

Create NFS directory:
```bash
sudo mkdir -p /srv/nfs4/bb_busybox
sudo chown -R root:root /srv/nfs4/bb_busybox
```

Start NFS services:
```bash
sudo systemctl enable nfs-kernel-server
sudo systemctl start nfs-kernel-server
sudo exportfs -ra
```

### 3. Verify Services

Check TFTP:
```bash
sudo systemctl status tftpd-hpa
```

Check NFS:
```bash
sudo systemctl status nfs-kernel-server
sudo exportfs -v
```

Test TFTP from another machine:
```bash
tftp 192.168.1.11
get am335x-boneblack.dtb
quit
```

Test NFS mount:
```bash
sudo mount -t nfs 192.168.1.11:/srv/nfs4/bb_busybox /mnt
ls /mnt
sudo umount /mnt
```

## Deployment Scripts

### DevTool Workflow (Recommended)

The preferred method uses Yocto's `devtool` for kernel development:

```bash
# Complete build and deploy
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh all
```

**Manual devtool commands:**
```bash
cd /opt/yocto/ycoto-excersise
./bitbake_bb_rpi -h bb -d sysv -s
source poky/5.0.14/oe-init-build-env bb-build-sysv

# Set up kernel workspace (first time only)
devtool modify linux-bb.org

# Edit kernel source at:
# workspace/sources/linux-bb.org/

# Build and deploy
devtool build linux-bb.org
devtool deploy-target linux-bb.org /srv/tftp
```

### Individual Deployments
```bash
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh all
```

### Individual Deployments
```bash
# Build only (devtool modify + build)
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh build

# Deploy only (devtool deploy-target)
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh deploy

# Deploy kernel modules
sudo ../../prepareEnv/bb-setup/scripts/deploy_modules.sh

# Deploy complete rootfs
sudo ../../prepareEnv/bb-setup/scripts/deploy_network_boot.sh rootfs
```

## Troubleshooting

### Common Issues

1. **TFTP Timeout**
   - Check firewall: `sudo ufw allow 69/udp`
   - Verify TFTP service: `sudo systemctl status tftpd-hpa`
   - Test connectivity: `ping 192.168.1.11`

2. **NFS Mount Fails**
   - Check NFS service: `sudo systemctl status nfs-kernel-server`
   - Verify exports: `sudo exportfs -v`
   - Check firewall: `sudo ufw allow nfs`

3. **Boot Hangs at "Loading kernel"**
   - Verify kernel file exists: `ls -la /srv/tftp/zImage_native_bb`
   - Check DTB file: `ls -la /srv/tftp/am335x-boneblack.dtb`
   - Verify file permissions (should be readable)

4. **Root filesystem mount fails**
   - Check NFS export path: `/srv/nfs4/bb_busybox`
   - Verify bootargs in U-Boot environment
   - Ensure NFS root contains valid filesystem

### Debug Commands

On BeagleBone Black console:
```bash
# Check network interface
ifconfig eth0

# Check NFS mount
mount | grep nfs

# Check loaded modules
lsmod

# View kernel messages
dmesg | tail -20

# Check filesystem
df -h
```

On host server:
```bash
# Check TFTP logs
sudo journalctl -u tftpd-hpa -f

# Check NFS connections
sudo netstat -an | grep :2049

# Monitor network boot
sudo tcpdump -i eth0 host 192.168.1.107
```

## File Structure

```
Host Server (192.168.1.11):
├── /srv/tftp/
│   ├── zImage_native_bb        # BeagleBone kernel
│   └── am335x-boneblack.dtb   # Device tree blob
└── /srv/nfs4/bb_busybox/      # NFS root filesystem
    ├── bin/
    ├── lib/
    │   └── modules/           # Kernel modules
    ├── etc/
    ├── root/
    └── ...

BeagleBone Black (192.168.1.107):
└── / (mounted via NFS from 192.168.1.11:/srv/nfs4/bb_busybox)
```
