#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ -f "$DIR/disk_image" ]
then
	echo "$file found."
else
	dd if=/dev/zero of=$DIR/disk_image bs=512 count=32768
    mkfs.fat -F 16 $DIR/disk_image
fi

dd if=$DIR/build/bootsector/bootsector of=$DIR/disk_image bs=1 count=3 conv=notrunc #sometimes it truncates drive o.O
dd if=$DIR/build/bootsector/bootsector of=$DIR/disk_image bs=62 seek=1 skip=1 conv=notrunc
mcopy -i $DIR/disk_image $DIR/build/loader/loader :: -D o
mcopy -i $DIR/disk_image $DIR/build/kernel/kernel :: -D o
echo 1 | sudo -S bash -c "echo 1 > /proc/sys/vm/drop_caches"
