#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>
#include <unistd.h>
#include "6474.h"
#include "motor_options.h"

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
void send_data_raw(char *pdata, int num_byte, int base_addr)
{
	unsigned char data, status, control;
	int data_addr = base_addr;
	int status_addr = base_addr + 1;
	int control_addr = base_addr  + 2;
	int n, bit, tmp;

	/*
	 * on entry data port must already be 0,
	 * HOST_CS must be 0
 	 */
	data = 0;
	outb_p(data, data_addr);
	msleep(1);

	/* start sending raw data */
	for (n=0; n<num_byte; n++){
		tmp = pdata[n];

		/* send a byte */
		for(bit=7; bit>=0; bit--){

			/* set clock low */
			data &= (~HOST_CLK);
			outb_p(data, data_addr);
			msleep(1);

			/* set SDO bit banding value */
			if (pdata[n] & (1<<bit)){
				data |= HOST_SDO;
			}
			else{
				data &= (~HOST_SDO);
			}
			outb_p(data, data_addr);
			msleep(1);

			/* set clock high */
			data |= HOST_CLK;
			outb_p(data, data_addr);
			msleep(1);

			if (bit == 0){
				msleep(20);
			}
		}

	}
	/* set data port back to 0  */
	data = 0;
	outb_p(data, data_addr);
	msleep(1);
}

void rx_data(char *pdata, int num_byte, int base_addr)
{
	unsigned char data, status, control;
	int data_addr = base_addr;
	int status_addr = base_addr + 1;
	int control_addr = base_addr  + 2;
	int n, bit, tmp;
	char header[] = HOST_READ_CODE;

	/* initilize parallel port to known state */
	status = inb_p(status_addr);
	control = inb_p(control_addr);
	outb_p(data, data_addr);
	msleep(1);

	/* data port = 0x00 */
	data = 0;
	outb_p(data, data_addr);
	msleep(1);

	/* set CS high and wait for 100ms for reset, probe pin is inverted */
	control &= (~HOST_CS);
	outb_p(control, control_addr);
	msleep(100);

	/* set CS low, probe pin is inverted */
	control |= HOST_CS;
	outb_p(control, control_addr);
	msleep(1);

	/* send read request header,
	 * on entry data port must be 0,
	 * HOST_CS must be low
	 */
	send_data_raw(header, 4, base_addr);

	/* receive data */
	for (n=0; n<num_byte; n++){

		/* receive a byte */
		for(bit=7; bit>=0; bit--){

			/* set clock low */
			data &= (~HOST_CLK);
			outb_p(data, data_addr);
			msleep(1);

			/* set clock high */
			data |= HOST_CLK;
			outb_p(data, data_addr);
			msleep(1);

			/* wait for reply */
			if (bit == 7){
				/* for each words, pause 100ms for reading from EEPROM */
				msleep(20);
			}

			/* check SDI bit value */
			status = inb_p(status_addr);
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
	outb_p(data, data_addr);
	msleep(1);

	/* set CS high, probe pin is inverted */
	control &= (~HOST_CS);
	outb_p(control, control_addr);
	msleep(1);

}


void send_data(char *pdata, int num_byte, int base_addr)
{
	unsigned char data, status, control;
	int data_addr = base_addr;
	int status_addr = base_addr + 1;
	int control_addr = base_addr  + 2;
	int n, bit, tmp;
	char header[] = HOST_WRITE_CODE;

	/* initilize parallel port to known state */
	status = inb_p(status_addr);
	control = inb_p(control_addr);
	outb_p(data, data_addr);

	/* data port = 0x00 */
	data = 0;
	outb_p(data, data_addr);
	msleep(1);

	/* set CS high and wait for 100ms for reset, probe pin is inverted */
	control &= (~HOST_CS);
	outb_p(control, control_addr);
	msleep(100);

	/* set CS low, probe pin is inverted */
	control |= HOST_CS;
	outb_p(control, control_addr);
	msleep(1);

	/* send header */
	send_data_raw(header, 4, base_addr);

	/* send data */
	send_data_raw(pdata, num_byte, base_addr);

	/* set CS high, probe pin is inverted */
	control &= (~HOST_CS);
	outb_p(control, control_addr);
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
 * main entry
 */
int main(int argc, char **argv)
{
	int n;
	int result;
	int base_addr = 0x378;
	char data[4+EEPROM_MAX_BYTE];
	struct motor_options p;

	/* options */
	if (!get_motor_options(argc, argv, &p)){
		exit(0);
	}

	/* allow access to whole address space */
	result = iopl(3);
	if (result){
		printf("failed to allow acesss the entire address space\n");
		printf("usage: sudo %s \n", argv[0]);
		exit(0);
	}

	/* set io permission */
	result = ioperm(base_addr,5,1);
	if (result){
		printf("failed to set permission for address %04x\n", base_addr);
		exit(0);
	}

	/* prepare data */
	memset(data, 0, EEPROM_MAX_BYTE);
	for(n=0; n<EEPROM_MAX_BYTE; n++){
		data[n] = n;
	}
	/* send data test */
	send_data(data, EEPROM_MAX_BYTE, base_addr);

	/* receive data test */
	memset(data, 0, EEPROM_MAX_BYTE);
	rx_data(data, EEPROM_MAX_BYTE, base_addr);
	dump_data(data, EEPROM_MAX_BYTE);
}
