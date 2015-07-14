#!/bin/bash
VENDOR=8086
DEVICE=2829
RENA_HOME="RenaElectronics"
LOG="${RENA_HOME}/`date +%F_%T`.log"

# create a home directory to store information
mkdir ~/${RENA_HOME} 2> /dev/null

# Find IO address
IO_ADDRESS=`lspci -d ${VENDOR}:${DEVICE} -v | grep -i -o  "I/O ports at .*" | head -n 1 | cut -d ' ' -f 4`
if [ -z $IO_ADDRESS ]; then
	echo "Failed to find IO address" | tee ${LOG}
fi
echo "Found PCIe ${VENDOR}:${DEVICE} IO address at ${IO_ADDRESS}" | tee ${LOG}




