#ifndef PARPORT_H
#define PARPORT_H

void send_data(char *pdata, int num_byte, int fd);
void rx_data(char *pdata, int num_byte, int fd);
void dump_data(char *pdata, int size);

int parport_init(char *devname);
void parport_exit(int fd);

#endif
