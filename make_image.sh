#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
dd if=${DIR}/build/bootsector/bootsector of=${DIR}/usb_image bs=1 count=3 conv=notrunc #sometimes it truncates drive o.O
dd if=${DIR}/build/bootsector/bootsector of=${DIR}/usb_image bs=62 seek=1 skip=1 conv=notrunc
mcopy -i ${DIR}/usb_image ${DIR}/build/loader/loader :: -D o
mcopy -i ${DIR}/usb_image ${DIR}/build/kernel/kernel :: -D o
echo 1 | sudo -S bash -c "echo 1 > /proc/sys/vm/drop_caches"
