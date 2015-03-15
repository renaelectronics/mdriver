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
int pp_write_read_data(unsigned char *pchar, int fd){
	unsigned char echo;
	int rc;

	/* write data */
	write(fd, pchar, 1);

	/* read data back */
	rc = read(fd, &echo, 1); 
	if (rc > 0){
		/* read back check */	
		if (*pchar != echo){
			printf("%s:read back error, sent %c, received %c\n", 
						__FUNCTION__, *pchar, echo);
			rc = -1;
        }
	}
    else{
		printf("%stimeout, sent %c\n", 	__FUNCTION__, *pchar);
    }	
	return rc;
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
