#!/bin/bash
# Custom init script

# Mount the SD card onto the /mnt directory
if [ -e /dev/mmcblk0p1 ]; then
	mount /dev/mmcblk0p1 /mnt
else
	echo "Insert SD card and mount to /mnt"
fi

# Initialise addresses on OCM that are shared with cpu1
rwmem 0xFFFF8004 0x0
rwmem 0xFFFF800C 0x0

# Start the cpu1 application
rwmem 0xFFFFFFF0 0x30000000
