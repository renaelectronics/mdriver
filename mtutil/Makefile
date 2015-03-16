# Copyright 2010 WCH GROUP all right reserved 
CC=gcc -Wall -O2 -pipe -o

all: mtutil

install: mtutil
	cp -p mtutil /usr/bin


mtutil: mtutil.c
	$(CC) mtutil mtutil.c options.c serial.c	

clean:  
	rm -f mtutil
	rm -f *~ *.o

