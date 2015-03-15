#ifndef SERIAL_H
#define SERIAL_H

int serial_open(char * device_name);
void serial_close(int fd);

#endif
