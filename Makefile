# Copyright 2010 WCH GROUP all right reserved 
all: wchdriver_make wchdump_make wch6474_make

install: wchdriver_install wchdump_install wchmknod_install wch6474_install

clean: wchdriver_clean wchdump_clean wchmknod_clean local_clean wch6474_clean

wchdriver_make:
	cd wchdriver;\
	make

wchdriver_install:
	cd wchdriver;\
	make install

wchdriver_clean:
	cd wchdriver;\
	make clean
	
wchdump_make:
	cd wchdump;\
	make

wchdump_install:
	cd wchdump;\
	make install

wchdump_clean:
	cd wchdump;\
	make clean;

wchmknod_install:
	cd wchmknod;\
	./wchmknod

wchmknod_clean:
	cd wchmknod;\
	rm -f *~

wch6474_make:
	cd wch6474;\
	make

wch6474_install:
	cd wch6474;\
	make install

wch6474_clean:
	cd wch6474;\
	make clean;



local_clean:
	rm -f *~
