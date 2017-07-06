#ifndef PARPORT_H
#define PARPORT_H

void pulse_HOST_CS(int fd);
int get_HOST_SDI(int fd);
void send_packet(char *pdata, int num_byte, int fd);
void receive_packet(char *pdata, int num_byte, int fd);
void dump_data(char *pdata, int size);
int parport_init(char *devname);
void parport_exit(int fd);

#endif
