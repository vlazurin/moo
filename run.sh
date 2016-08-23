#!/bin/bash
VBoxManage modifyvm test --hda none
VBoxManage closemedium disk sdb.vdi
rm ./sdb.vdi
VBoxManage convertdd usb_image sdb.vdi --format VDI
VBoxManage modifyvm test --hda sdb.vdi
VBoxManage modifyvm test --tracing-enabled on
VBoxManage startvm test
