#!/bin/sh

# Continuously monitor threads and update flags accordingly
while true
do
	rwmem 0x43c00000
	rwmem 0x43c00004
	rwmem 0x43c00008
	echo -e "\n"
	sleep 2
done
