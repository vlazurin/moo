#!/bin/bash
clear

bochs=0

while getopts "b" opt; do
    case "$opt" in
    b) bochs=1
    ;;
    esac
done

ssh -t vasiliy@127.0.0.1 -p3022 'cd /media/sf_moo/build; make'
result=$?;
if [[ $result = 0 ]]; then
    if [[ $bochs = 0 ]]; then
        ./run.sh
    else
        echo "c" | bochs -f bochs_config -q
    fi
fi
