#!/bin/sh
echo "### Setting driver for pick and place machine ###"
sudo ./motordrv -m 1 -c 2 -s 3 -w 30
sudo ./motordrv -m 3 -c 2 -s 3 -w 30
sudo ./motordrv -m 2 -c 1.5 -s 3
sudo ./motordrv -m 4 -c 2.8 -s 0
