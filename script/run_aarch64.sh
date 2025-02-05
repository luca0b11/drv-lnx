#!/bin/bash
QEMU="/home/francesco/Desktop/buildRoot/qemu/build/aarch64-softmmu/qemu-system-aarch64"
KERNEL="/home/francesco/Desktop/buildRoot/buildroot/output/images"
exec $QEMU -M virt -cpu cortex-a53 -nographic -smp 1 -kernel $KERNEL/Image -append "rootwait root=/dev/vda console=ttyAMA0" -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 -drive file=$KERNEL/rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -d mmu
