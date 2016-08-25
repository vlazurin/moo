#!/bin/bash
VM='moo'
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

VBoxManage modifyvm $VM --hda none
VBoxManage closemedium disk $DIR/sdb.vdi
rm $DIR/sdb.vdi
VBoxManage convertdd $DIR/disk_image $DIR/sdb.vdi --format VDI
VBoxManage modifyvm $VM --hda $DIR/sdb.vdi
#VBoxManage modifyvm $VM --nictrace1 on --nictracefile1 file.pcap
VBoxManage startvm $VM
