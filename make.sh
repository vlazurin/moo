#!/bin/bash
clear

bochs=0
mdm=0

while getopts "mb" opt; do
    case "$opt" in
    b) bochs=1
    ;;
    m) mdm=1
    ;;
    esac
done

ssh -t vasiliy@127.0.0.1 -p3023 'cd /media/sf_moo/build; make'
result=$?;
if [[ $result = 0 ]]; then
    if [[ $mdm = 1 ]]; then
        ./build/userspace/mdm/mdm
    else
        if [[ $bochs = 0 ]]; then
            ./run.sh
        else
            echo "c" | bochs -f bochs_config -q
        fi
    fi
fi
