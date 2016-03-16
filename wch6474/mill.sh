#!/bin/sh
echo "### Setting driver for milling  machine ###"
sudo ./wch6474 -m 1 -c 1.5 -s 1
sudo ./wch6474 -m 2 -c 1.5 -s 1
sudo ./wch6474 -m 3 -c 1.5 -s 1
