#!/bin/bash
VENDOR=1c00
DEVICE=3250
RENA_HOME="${HOME}/RenaElectronics"
LOG="${RENA_HOME}/`date +%F_%T`.log"
TOTAL_LOG_FILES=1
WCH_DRIVER=wch
TEE="tee -a ${LOG}"
UNAME=`uname -r`
STEPCONF_FILE=linuxcnc-stepconf-rena.desktop
PARPORT_DRIVERS='parport_ax88796.ko parport_cs.ko parport.ko parport_pc.ko parport_serial.ko'

# create a home directory to store information
mkdir ${RENA_HOME} 2> /dev/null

# keep last log
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
	echo "Make sure your PCIe card is installed correctly and try again." | ${TEE}
	exit -1
fi
echo "Found PCIe ${VENDOR}:${DEVICE} IO address at ${IO_ADDRESS}" | ${TEE} 

# ----------------------------------
#  wget files and save to RENA_HOME
# ----------------------------------
echo "wget files and save to ${RENA_HOME}" | ${TEE}
rm -f ${RENA_HOME}/${WCH_DRIVER}.ko
rm -f ${RENA_HOME}/${STEPCONF_FILE}
wget -P ${RENA_HOME} http://www.renaelectronics.com/drivers/${WCH_DRIVER}.ko | ${TEE}
wget -P ${RENA_HOME} http://www.renaelectronics.com/drivers/${STEPCONF_FILE} | ${TEE}

# ------------------------------------------------
# insmod WCH serial module
# ------------------------------------------------
echo -n "Installing WCH driver, please wait ." | ${TEE}
sudo modprobe -r ${WCH_DRIVER} | ${TEE}
echo -n "." | ${TEE}
sudo mkdir -p /lib/modules/${UNAME}/kernel/drivers/char/ | ${TEE}
echo -n "." | ${TEE}
sudo cp -f ${RENA_HOME}/${WCH_DRIVER}.ko /lib/modules/${UNAME}/kernel/drivers/char/ | ${TEE}
echo -n "." | ${TEE}
sudo mkdir -p /lib/modules/${UNAME}/misc/ | ${TEE}
echo -n "." | ${TEE}
sudo cp -f ${RENA_HOME}/${WCH_DRIVER}.ko /lib/modules/${UNAME}/misc/ | ${TEE}
echo -n "." | ${TEE}
sudo depmod -a | ${TEE}
echo -n "." | ${TEE}
sudo modprobe ${WCH_DRIVER} | ${TEE}
echo -n "."
if [ -z "lsmod | grep ${WCH_DRIVER}" ]; then
	echo " failed to install ${WCH_DRIVER}" | ${TEE}
	exit -1
fi
echo " completed" | ${TEE}
echo "Build tty file node (ttyWCH0 ~ ttyWCH3)" | ${TEE}
port=0
while [ $port -lt 4 ]
do
    sudo rm -f /dev/ttyWCH$port | ${TEE}
    echo /dev/ttyWCH$port | ${TEE}
    sudo mknod /dev/ttyWCH$port c 245 $port | ${TEE}
    sudo chmod a+w /dev/ttyWCH$port | ${TEE}
    port=`expr $port + 1`
done

# ----------------------------------------------------------
#   install parport driver to support WCH PCIe parport card
# ----------------------------------------------------------
for i in ${PARPORT_DRIVERS}
do 
	rm -f ${RENA_HOME}/${i}
	wget -P ${RENA_HOME} http://www.renaelectronics.com/drivers/${i}
	sudo cp ${RENA_HOME}/${i} /lib/modules/3.4-9-rtai-686-pae/kernel/drivers/parport
done

# ----------------------------------------------------------------------
#  create stepconf file
#  original menu file /usr/share/applications/linuxcnc-stepconf.desktop
# ----------------------------------------------------------------------
sudo cp ${RENA_HOME}/${STEPCONF_FILE}  /usr/share/applications/

echo "Please reboot to activate the changes"
exit 0

