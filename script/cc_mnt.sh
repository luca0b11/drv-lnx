#!/bin/bash

test_file="${1:-/home/luca/polito_ws/os_ws/project/group8/test/aarch64_v2/main.c}"
rootfs="${2:-/home/luca/polito_ws/os_ws/project/buildroot/output/images/rootfs.ext4}"
matrix_a="${3:-/home/luca/polito_ws/os_ws/project/group8/test/aarch64_v2/matrix_a.txt}"
matrix_b="${4:-/home/luca/polito_ws/os_ws/project/group8/test/aarch64_v2/matrix_b.txt}"

mnt_dir="/mnt/rootfs_mount"
output_name="test_driver_v2"

# Check sudo permission
if ! sudo -v; then
    echo "Error: This script requires sudo permissions."
    exit 1
fi

# Check test file exist
if [ ! -f "$test_file" ]; then
    echo "Error: test file '$test_file' does not exist"
    exit 1
fi

# Compile test file
aarch64-linux-gnu-gcc -mcpu=cortex-a53 "$test_file" -o "$output_name" || { echo "Error: compilation of '$test_file' failed"; exit 1; }

# Check rootfs exist
if [ ! -f "$rootfs" ]; then
    echo "Error: imagine rootfs '$rootfs' does not exist."
    exit 1
fi

# Make a clean directory to mount filesystem
if [ -d "$mnt_dir" ]; then
    sudo rm -rf "$mnt_dir"
fi
sudo mkdir -p "$mnt_dir"

# Mount filesystem
sudo mount -o loop "$rootfs" "$mnt_dir" || { echo "Error: filesystem mount failed"; exit 1; }

sudo chmod 777 "$mnt_dir"/root

# Clear filesystem /root
sudo rm -rf "$mnt_dir"/root/* || { echo "Error: rootfs /root clear failed"; sudo umount "$mnt_dir"; exit 1; }

# Copy executable test file to /root
sudo cp "$output_name" "$mnt_dir"/root || { echo "Error: executable file copy failed"; sudo umount "$mnt_dir"; exit 1; }

sudo cp "$matrix_a" "$mnt_dir"/root || { echo "Error: txt file copy failed"; sudo umount "$mnt_dir"; exit 1; }

sudo cp "$matrix_b" "$mnt_dir"/root || { echo "Error: txt file copy failed"; sudo umount "$mnt_dir"; exit 1; }

# Unmount and clear
sudo umount "$mnt_dir"
sudo rm -rf /mnt
rm "$output_name"
