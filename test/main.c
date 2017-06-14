#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

/* remember to modprobe ppdev */

int running = 1;

void signalHandler(int sig)
{
   running = 0;
}

/* 
 * debug
 */
void dprint(char data)
{
	int n;
	for (n=7; n>=0; n--){
		if (data & (1<<n))
			printf("%c", '1');
		else
			printf("%c", '0');
	}
	printf("\n");
}


/* 
 * input pin, status pin
 */
int get_inpin(int fd)
{
	char status;
	ioctl(fd, PPRSTATUS, &status);
	if (status & PARPORT_STATUS_ACK)
		return 1;
	return 0;
}

/*
 * set and clr pin
 */
#define CLK		(0)
#define OUTPIN	(1)
#define CS		(2)

int set_pin(int fd, int data, int pin)
{
	data = data | (1<<pin);
	dprint(data);
	ioctl(fd, PPWDATA, &data);
	return data;
}

int clr_pin(int fd, int data, int pin)
{
	data = data & ~(1<<pin);
	dprint(data);
	ioctl(fd, PPWDATA, &data);
	return data;
}

/*
 * pulse cs
 */
int pulse_cs(int fd)
{
	int data = 0;

	data = clr_pin(fd, data, CS);
	usleep(1000);

	data = set_pin(fd, data, CS);
	usleep(1000);
	
	data = clr_pin(fd, data, CS);
	usleep(1000);

}

/* 
 * serialize the data and send it out
 * return number of bit sent or -1 
 */
int writebyte(int fd, char c)
{
	int n;
	char data = 0;

	if (get_inpin(fd) != 0)
		return -1;

	for (n=0; n<8; n++){

		/* CLK = 0 */
		data = clr_pin(fd, data, CLK);
		usleep(1000);

		/* OUTPIN = 1,0 */
		if (c & (1<<n))
			data = set_pin(fd, data, OUTPIN);
		else
			data = clr_pin(fd, data, OUTPIN);
		usleep(1000);

		/* CLK = 1 */
		data = set_pin(fd, data, CLK);
		usleep(1000);
	
	}
	data = clr_pin(fd, data, CLK);
	data = clr_pin(fd, data, OUTPIN);
	usleep(1000);		
	return n;
}

char readbyte(int fd)
{
	int n;
	char data = 0;
	char c = 0;

	if (get_inpin(fd) != 1)
		return -1;

	/* CLK = 0 */
	data = clr_pin(fd, data, CLK);
	usleep(1000);

	for (n=0; n<8; n++){

		/* CLK = 1 */
		data = set_pin(fd, data, CLK);
		usleep(1000);

		/* INPIN = 1,0 */
		if (get_inpin(fd))
			c = c | (1<<n);
		else
			c = c & ~(1<<n);
		usleep(1000);

		/* CLK = 0 */
		data = clr_pin(fd, data, CLK);
		usleep(1000);
	
	}
	data = clr_pin(fd, data, CLK);
	usleep(1000);		
	return c;
	
}

int main(void)
{
   int fd;
   int result;
   int i;

   printf("Parallel Port Interface Test\n");

   // Set ctrl-c handler
   signal(SIGINT, signalHandler);

   // Open the parallel port for reading and writing
   fd = open("/dev/parport0", O_RDWR);

   if (fd == -1)
   {
      perror("Could not open parallel port, remember to modprobe ppdev");
      return 1;
   }

   // Try to claim port
   if (ioctl(fd, PPCLAIM, NULL))
   {
      perror("Could not claim parallel port");
      close(fd);
      return 1;
   }

   // Set the Mode
   int mode = IEEE1284_MODE_BYTE;
   if (ioctl(fd, PPSETMODE, &mode))
   {
      perror("Could not set mode");
      ioctl(fd, PPRELEASE);
      close(fd);
      return 1;
   }

   // Set data pins to output
   int dir = 0x00;
   if (ioctl(fd, PPDATADIR, &dir))
   {
      perror("Could not set parallel port direction");
      ioctl(fd, PPRELEASE);
      close(fd);
      return 1;
   }

   char dataH = 0xFF;
   unsigned char dataL = 0x00;
   char ctrl = 0x00;
   char status = 0x00;

   // read and write enable strobe
   ioctl(fd, PPRCONTROL, &ctrl);
   ctrl = (ctrl | PARPORT_CONTROL_STROBE);
   ioctl(fd, PPWCONTROL, &ctrl);

#if (0)
   // This loop will keep running until ctrl-c is pressed
   for (i=0; i<10; i++) 
   {
      	// Output some data
      	// Note that data is passed by a pointer
	printf("data = %02x\n", dataH); fflush(stdout);
      	ioctl(fd, PPWDATA, &dataH);
	usleep(1000);

	// Read ACK, PPRSTATUS bit 6
	ioctl(fd, PPRSTATUS, &status);
	printf("status = %02x, ACK = %02x\n", status, status & PARPORT_STATUS_ACK);
	dataH = ~dataH;
   }
#endif

	printf("\n");
	pulse_cs(fd);
	dataL = 0xca;
	if (writebyte(fd, dataL) != 8){
		printf("writebyte error!\n");
	}
	dataL = 0;
	dataL = readbyte(fd);
	printf("readbyte=%02x\n", dataL);

   printf("Shutting down\n");

   // Release and close the parallel port
   ioctl(fd, PPRELEASE);
   close(fd);

   return 0;
}
