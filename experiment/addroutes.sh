#!/bin/bash
while IFS='' read -r line || [[ -n "$line" ]]; do
    array=($line)	
    echo "Text read from file: ${array[0]} ${array[1]}"
#    sudo ip route add ${array[0]} table main dev ${array[1]}
    sudo ip route add ${array[0]} table main dev wlo1
done < "$1"
