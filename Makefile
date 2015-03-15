# Copyright 2010 WCH GROUP all right reserved 
all: wchdriver_make wchdump_make mtutil_make

install: wchdriver_install wchdump_install wchmknod_install mtutil_install

clean: wchdriver_clean wchdump_clean wchmknod_clean local_clean mtutil_clean

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

mtutil_make:
	cd mtutil;\
	make

mtutil_install:
	cd mtutil;\
	make install

mtutil_clean:
	cd mtutil;\
	make clean;



local_clean:
	rm -f *~
