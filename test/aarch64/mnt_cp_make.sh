sudo mount -o loop /home/francesco/Desktop/buildRoot/buildroot/output/images/rootfs.ext4 /mnt/rootfs_mount

cd /home/francesco/Desktop/gitLab/group8/TEST/aarch64

make main_arm

cp ./main_arm /mnt/rootfs_mount/root/
cp ./main.c /mnt/rootfs_mount/root/

sudo umount /mnt/rootfs_mount
