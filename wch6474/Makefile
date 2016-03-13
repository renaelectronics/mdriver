# Copyright 2010 WCH GROUP all right reserved 
CC=gcc -Wall -O2 -pipe -o

all: wch6474

install: wch6474
	cp -p wch6474 /usr/bin


wch6474: wch6474.c
	$(CC) wch6474 wch6474.c options.c serial.c	

clean:  
	rm -f wch6474
	rm -f *~ *.o

