#!/bin/bash
VENDOR=1c00
DEVICE=3250
RENA_HOME="${HOME}/RenaElectronics"
LOG="${RENA_HOME}/`date +%F_%T`.log"
TOTAL_LOG_FILES=1
WCH_DRIVER=wch
TEE="tee -a ${LOG}"
UNAME=`uname -r`

# create a home directory to store information
mkdir ${RENA_HOME} 2> /dev/null

# keep last 10 log
LOG_FILES=`ls ${RENA_HOME}/*.log 2> /dev/null` 
COUNT=`echo ${LOG_FILES} | wc -w`
COUNT=$((COUNT - ${TOTAL_LOG_FILES}))
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
	echo "Failed to find ${VENDOR}:${DEVICE} IO address" | ${TEE}
	echo "Make sure your PCIe card is installed correctly and try again." | tee -a ${LOG}
	exit -1
fi
echo "Found PCIe ${VENDOR}:${DEVICE} IO address at ${IO_ADDRESS}" | tee -a ${LOG}

# -----------------------------------
#  wget and insmod WCH serial module
# -----------------------------------
echo "log existing wch module if exists" | tee -a ${LOG}
lsmod | grep wch | tee -a ${LOG}
echo "wget wch module and save it to ${RENA_HOME}"
rm -f ${RENA_HOME}/${WCH_DRIVER}.ko
wget -P ${RENA_HOME} http://www.renaelectronics.com/drivers/${WCH_DRIVER}.ko | tee -a ${LOG}
echo -n "Installing WCH driver, please wait ."
sudo modprobe -r ${WCH_DRIVER}
echo -n "."
sudo mkdir -p /lib/modules/${UNAME}/kernel/drivers/char/
echo -n "."
sudo cp -f ${RENA_HOME}/${WCH_DRIVER}.ko /lib/modules/${UNAME}/kernel/drivers/char/
echo -n "."
sudo mkdir -p /lib/modules/${UNAME}/misc/
echo -n "."
sudo cp -f ${RENA_HOME}/${WCH_DRIVER}.ko /lib/modules/${UNAME}/misc/
echo -n "."
sudo depmod -a
echo -n "."
sudo modprobe ${WCH_DRIVER}
echo -n "."
if [ -z "lsmod | grep ${WCH_DRIVER}" ]; then
	echo " failed to install ${WCH_DRIVER}"
	exit -1
fi
echo " completed"

# ----------------------
#  create stepconf file
# ----------------------

exit 0

