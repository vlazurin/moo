#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VM='moo'
VBoxManage createvm --name $VM --ostype "Other" --register
VBoxManage storagectl $VM --name "IDE Controller" --add ide
VBoxManage modifyvm $VM --memory 1024 --vram 128
VBoxManage modifyvm $VM --nic1 nat --nictype1 82540EM
VBoxManage modifyvm $VM --nictrace1 on --nictracefile1 $DIR/net_trace.pcap
VBoxManage modifyvm $VM --uart1 0x3F8 4
VBoxManage modifyvm $VM --uartmode1 file $DIR/debug.txt
