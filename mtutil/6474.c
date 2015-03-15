#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/ppdev.h>
#include "6474.h"
#include "serial.h"

/* write data and wait for echo */
int serial_write_read_data(int fd, char *pchar, int size){
	char echo;
	int rc;
	int n;

	for (n=0; n<size; n++){
		/* write data */
		write(fd, &pchar[n], 1);
		/* read data back */
		rc = read(fd, &echo, 1); 
		if (rc > 0){
			/* read back check */	
			if (pchar[n] != echo){
				printf("\n%s : read back error, sent %02x, received %02x\n", 
							__FUNCTION__, (unsigned char)pchar[n], echo);
				return -1;
        	}
		}
    	else{
			printf("\n%s : timeout, sent %d of %d, %02x\n",
				__FUNCTION__, n, size-1, pchar[n]);
			return -1;
    	}	
	}
	return size;
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
