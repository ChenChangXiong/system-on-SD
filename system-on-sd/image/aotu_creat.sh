#!/bin/sh


sudo dd if=/dev/zero of=SD-201900821.img  bs=1M count=150
sudo losetup -f --show SD-201900821.img
sudo fdisk  /dev/loop0  <<EOF

n
p
1
2048
204799
t
e
a
1
p
w
EOF

sleep 1

sudo kpartx -av /dev/loop0
sudo mkfs.vfat    -n "boot" -F 16  /dev/mapper/loop0p1

sudo mount /dev/mapper/loop0p1 /mnt
sudo cp uEnv.txt  MLO u-boot.img zImage  ubi.img  am335x-ariio_5702.dtb /mnt -rf
sync


sudo umount /mnt

sudo  kpartx -d /dev/loop0
sudo  losetup -d /dev/loop0


echo "==create finish==="
rm ~/SD* -rf
sync 
cp SD* ~/
sync
echo "==copy finish==="

