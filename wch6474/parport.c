#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include "6474.h"
#include "parport.h"

/*
 * parallel port status register, base + 1
 */
#define STATUS_ERROR	(0x08)	/* pin 15 */
#define STATUS_SELECT	(0x10)	/* pin 13 */
#define STATUS_PAPER_OUT (0x20)	/* pin 12 */
#define STATUS_ACK	(0x40)	/* pin 10 */
#define STATUS_BUSY	(0x80)	/* pin 11 */

/*
 * parallel port control register, base + 2
 */
#define CTRL_STROBE	(0x01)	/* pin 1 */
#define CTRL_AUTOFEED	(0x02)	/* pin 14 */
#define CTRL_INIT	(0x04)	/* pin 16 */
#define CTRL_SELECT_IN	(0x08)	/* pin 17 */

/* host bit banding */
#define HOST_STROBE	(CTRL_STROBE)	/* crtl addr, bit 0 */
#define HOST_CLK	(0x01)	/* data addr, bit 0 */
#define HOST_SDO	(0x02)	/* data addr, bit 1 */
#define HOST_CS		(0x04)	/* data addr, bit 2 */

/* set/get methods for bit array
 *
 *                   1         2
 *   pos : 012345678901234567890123
 * index : 0.......1.......2.......
 *
 */
#define BIT_GET 0
#define BIT_SET 1
#define BIT_CLR 2

/* tx, rx packet */
const char tx_header[4] = {0xca, 0xfe, 0xba, 0xba};
const char rx_header[4] = {0xca, 0xfe, 0xbe, 0xbe};
char tx_packet[4 + EEPROM_MAX_BYTE];
char rx_packet[4 + EEPROM_MAX_BYTE];


/* pulse HOST_CS */
void pulse_HOST_CS(int fd)
{	
	unsigned char data = 0;

	/* HOST_CS = 0 */
	ioctl(fd, PPWDATA, &data);
	msleep(10);

	/* HOST_CS = 1 */
	data |= HOST_CS;
	ioctl(fd, PPWDATA, &data);
	msleep(10);


	/* HOST_CS = 0 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(10);
}

/*
 * return the status of SDI
 */
int get_HOST_SDI(int fd)
{
	unsigned char status;

	/* check SDI bit value */
	ioctl(fd, PPRSTATUS, &status);
	if (status & PARPORT_STATUS_ACK)
		return 1;
	else 
		return 0;
}

/*
 * send raw data, on entry, HOST_STROBE must be low, data port must be 0
 */
void send_packet_raw(char *pdata, int num_byte, int fd)
{
	unsigned char data;
	char *ptr;
	int n, bit;

	/* data port = 0 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(50);

	/* start sending raw data */
	for (n=0; n<num_byte; n++){
		ptr = &pdata[n];
		for(bit=0; bit<8; bit++){

			/* set clock low */
			data &= (~HOST_CLK);
			ioctl(fd, PPWDATA, &data);
			msleep(100);

			/* set SDO bit banding value */
			if (ptr[0] & (1<<bit)){
				data |= HOST_SDO;
			}
			else{
				data &= (~HOST_SDO);
			}
			ioctl(fd, PPWDATA, &data);
			msleep(100);

			/* set clock high */
			data |= HOST_CLK;
			ioctl(fd, PPWDATA, &data);
			msleep(100);
		}
	}
	/* wait for the packet to be process */
	msleep(1000);

	/* set data port back to 0  */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(10);
}

void send_packet(char *pdata, int num_byte, int fd)
{
	/* put 4 bytes header and copy pdata into tx_packet */
	memcpy(tx_packet, tx_header, 4);
	memcpy(tx_packet+4, pdata, num_byte);

	/* send packet out */
	send_packet_raw(tx_packet, 4 + EEPROM_MAX_BYTE, fd);
}

void receive_packet_raw(char *pdata, int num_byte, int fd)
{
	unsigned char data, status;
	char *ptr;
	int n, bit;

	/* get initializa status value */
	ioctl(fd, PPRSTATUS, &status);
	msleep(10);

	/* data port = 0x00 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(10);

	/* receive data */
	for (n=0; n<num_byte; n++){
		ptr = &pdata[n];
		for(bit=0; bit<8; bit++){
	
			/* set clock low */
			data &= (~HOST_CLK);
			ioctl(fd, PPWDATA, &data);
			msleep(10);

			/* set clock high */
			data |= HOST_CLK;
			ioctl(fd, PPWDATA, &data);
			msleep(10);

			/* check input data bit value */
			ioctl(fd, PPRSTATUS, &status);
			if (status & get_HOST_SDI(fd))
				ptr[0] |= (1<<bit);
			else
				ptr[0] &= (~(1<<bit));
		}
	}

	/* set data port back to 0 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(10);

}

/* send a read request and read back data
 * send : ca,fa,be,be,0,0...
 * read : ca,fa,be,be,n,n...
 */
void receive_packet(char *pdata, int num_byte, int fd)
{
	/* put 4 bytes header and copy pdata into tx_packet */
	memcpy(rx_packet, rx_header, 4);
	memcpy(rx_packet+4, pdata, num_byte);

	/* send the request out */
	send_packet_raw(rx_packet, 4 + EEPROM_MAX_BYTE, fd);

	/* read the data back */
	receive_packet_raw(rx_packet, 4 + EEPROM_MAX_BYTE, fd);
	memcpy(pdata, rx_packet+4, num_byte);
}

void dump_data(char *pdata, int size)
{
	int n;
	for(n=0;n<size;n++){
		if ((n%16) ==0){
			printf("\n");
		}
		printf("%02x ", pdata[n] & 0xff);
	}
	printf("\n");
}

/*
 * parallel port initiaization
 */
int parport_init(char *devname)
{
	int fd;
	int mode = IEEE1284_MODE_BYTE;
	int dir = 0x00;
	int control;

	// Open the parallel port for reading and writing
	fd = open(devname, O_RDWR);
	if (fd <= 0) {
		perror("Could not open parallel port, remember to modprobe ppdev");
		return -1;
	}

	// Try to claim port
	if (ioctl(fd, PPCLAIM, NULL)) {
		perror("Could not claim parallel port");
		close(fd);
		return -1;
	}

	// Set the Mode
	if (ioctl(fd, PPSETMODE, &mode)) {
		perror("Could not set mode");
		ioctl(fd, PPRELEASE);
		close(fd);
		return -1;
	}

	// Set data pins to output
	if (ioctl(fd, PPDATADIR, &dir)) {
		perror("Could not set parallel port direction");
		ioctl(fd, PPRELEASE);
		close(fd);
		return -1;
	}

	// make sure strobe is inactive, strobe is inverted
	ioctl(fd, PPRCONTROL, &control);
	control &= (~HOST_STROBE);
	ioctl(fd, PPWCONTROL, &control);
	msleep(10);

	return fd;
}

void parport_exit(int fd)
{
	// Release and close parallel port
	ioctl(fd, PPRELEASE);
	close(fd);
}
