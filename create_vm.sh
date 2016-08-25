#!/bin/bash
VM='moo'
VBoxManage createvm --name $VM --ostype "Other" --register
VBoxManage storagectl $VM --name "IDE Controller" --add ide
VBoxManage modifyvm $VM --memory 1024 --vram 128
VBoxManage modifyvm $VM --nic1 nat --nictype1 82540EM
