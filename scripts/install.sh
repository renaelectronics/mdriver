#!/bin/bash
VENDOR=1c00
DEVICE=3250
RENA_HOME="${HOME}/RenaElectronics"
LOG="${RENA_HOME}/`date +%F_%T`.log"

# create a home directory to store information
mkdir ${RENA_HOME} 2> /dev/null

# keep last 10 log
LOG_FILES=`ls ${RENA_HOME}/*.log 2> /dev/null` 
COUNT=`echo ${LOG_FILES} | wc -w`
COUNT=$((COUNT - 10))
for i in ${LOG_FILES}
do
	if [ ${COUNT} -ge 0 ] ;then
		rm $i
	fi
	COUNT=$((COUNT - 1))
done

# find IO address
IO_ADDRESS=`lspci -d ${VENDOR}:${DEVICE} -v | grep -i -o  "I/O ports at .*" | tail -n 1 | cut -d ' ' -f 4`
if [ -z $IO_ADDRESS ]; then
	echo "Failed to find ${VENDOR}:${DEVICE} IO address" | tee ${LOG}
	echo "Make sure your PCIe card is installed correctly and try again." | tee ${LOG}
	exit -1
fi
echo "Found PCIe ${VENDOR}:${DEVICE} IO address at ${IO_ADDRESS}" | tee ${LOG}

# wget and insmod WCH serial module

exit 0

