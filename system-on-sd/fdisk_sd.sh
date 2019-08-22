#!/bin/sh

if [ $# -lt "1" ];then
	echo ===fdisking===
	sudo fdisk /dev/sdb  <<EOF

	n
	p
	2
	204800
	15126527
	p
	w
EOF
	echo  =====fdisk finish=====
else
	sudo mkfs.ext4  /dev/sdb2
	sleep 3
	sudo mount  /dev/sdb2  /mnt/
#cd /mnt/
	sudo cp ~/Desktop/rootfs_yimi/*   /mnt/   -arf
	sleep 5
	sync
	cd -
	ls /mnt/
	sudo umount  /mnt/
	sync
	echo ===do finish===
fi
