#!/usr/bin/env bash
set -e

TARGET?=root@192.168.1.101
MOD_NAME?=mymod.ko

scp $MOD_NAME $TARGET:/tmp/
ssh $TARGET "rmmod mymod 2>/dev/null || true; insmod /tmp/$MOD_NAME; dmesg | tail -n 20"
