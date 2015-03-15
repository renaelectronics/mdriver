#!/bin/sh
echo "### Setting driver for milling  machine ###"
sudo ./motordrv -m 1 -c 1.5 -s 1
sudo ./motordrv -m 2 -c 1.5 -s 1
sudo ./motordrv -m 3 -c 1.5 -s 1
