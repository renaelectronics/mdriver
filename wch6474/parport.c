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

#define msleep(a)	do { printf("."); fflush(stdout); usleep(a*1000); } while (0)

/*
 * use outb_p(data, addr) and inb_p(addr) to write and read to/from parallel port
 */
#define HOST_WRITE_CODE {0xca, 0xfe, 0xba, 0x00}
#define HOST_READ_CODE	{0xca, 0xfe, 0xbe, 0x00}

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
#define HOST_CS		(CTRL_STROBE)	/* crtl addr, bit 0 */
#define HOST_CLK	(0x01)	/* data addr, bit 0 */
#define HOST_SDO	(0x02)	/* data addr, bit 1 */
#define HOST_SDI	(0x10)	/* status addr, bit 4 */

/*
 * send raw data, on entry, HOST_CS must be low, data port must be 0
 */
void send_data_raw(char *pdata, int num_byte, int fd)
{
	unsigned char data;
	int n, bit;

	/*
	 * on entry data port must already be 0,
	 * HOST_CS must be 0
 	 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(1);

	/* start sending raw data */
	for (n=0; n<num_byte; n++){
		/* send a byte */
		for(bit=7; bit>=0; bit--){

			/* set clock low */
			data &= (~HOST_CLK);
			ioctl(fd, PPWDATA, &data);
			msleep(1);

			/* set SDO bit banding value */
			if (pdata[n] & (1<<bit)){
				data |= HOST_SDO;
			}
			else{
				data &= (~HOST_SDO);
			}
			ioctl(fd, PPWDATA, &data);
			msleep(1);

			/* set clock high */
			data |= HOST_CLK;
			ioctl(fd, PPWDATA, &data);
			msleep(1);

			/* if last bit then wait for the data to process */
			if (bit == 0){
				msleep(20);
			}
		}

	}
	/* set data port back to 0  */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(1);
}

void rx_data(char *pdata, int num_byte, int fd)
{
	unsigned char data, status, control;
	int n, bit;
	char header[] = HOST_READ_CODE;

	/* initilize parallel port to known state */
	ioctl(fd, PPRSTATUS, &status);
	ioctl(fd, PPRCONTROL, &control);
	msleep(1);

	/* data port = 0x00 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(1);

	/* set CS high and wait for 100ms for reset, probe pin is inverted */
	control &= (~HOST_CS);
	ioctl(fd, PPWCONTROL, &control);
	msleep(100);

	/* set CS low, probe pin is inverted */
	control |= HOST_CS;
	ioctl(fd, PPWCONTROL, &control);
	msleep(1);

	/* send read request header,
	 * on entry data port must be 0,
	 * HOST_CS must be low
	 */
	send_data_raw(header, 4, fd);

	/* receive data */
	for (n=0; n<num_byte; n++){

		/* receive a byte */
		for(bit=7; bit>=0; bit--){

			/* set clock low */
			data &= (~HOST_CLK);
			ioctl(fd, PPWDATA, &data);
			msleep(1);

			/* set clock high */
			data |= HOST_CLK;
			ioctl(fd, PPWDATA, &data);
			msleep(1);

			/* wait for reply */
			if (bit == 7){
				/* for each words, pause 100ms for reading from EEPROM */
				msleep(20);
			}

			/* check SDI bit value */
			ioctl(fd, PPRSTATUS, &status);
			if (status & HOST_SDI){
				pdata[n] |= (1 <<  bit);
			}
			else{
				pdata[n] &= (~(1 << bit));
			}

		}
	}

	/* set data port back to 0 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(1);

	/* set CS high, probe pin is inverted */
	control &= (~HOST_CS);
	ioctl(fd, PPWDATA, &data);
	msleep(1);

}


void send_data(char *pdata, int num_byte, int fd)
{
	unsigned char data, status, control;
	char header[] = HOST_WRITE_CODE;

	/* initilize parallel port to known state */
	ioctl(fd, PPRSTATUS, &status);
	ioctl(fd, PPRCONTROL, &control);

	/* data port = 0x00 */
	data = 0;
	ioctl(fd, PPWDATA, &data);
	msleep(1);

	/* set CS high and wait for 100ms for reset, probe pin is inverted */
	control &= (~HOST_CS);
	ioctl(fd, PPWCONTROL, &control);
	msleep(100);

	/* set CS low, probe pin is inverted */
	control |= HOST_CS;
	ioctl(fd, PPWCONTROL, &control);
	msleep(1);

	/* send header */
	send_data_raw(header, 4, fd);

	/* send data */
	send_data_raw(pdata, num_byte, fd);

	/* set CS high, probe pin is inverted */
	control &= (~HOST_CS);
	ioctl(fd, PPWCONTROL, &control);
	msleep(1);
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
	int result;
	int mode = IEEE1284_MODE_BYTE;
	int dir = 0x00;

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

	return fd;
}

void parport_exit(int fd)
{
	// Release and close parallel port
	ioctl(fd, PPRELEASE);
	close(fd);
}
