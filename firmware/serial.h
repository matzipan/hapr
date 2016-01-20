#ifndef _HAPR_SERIAL_H
#define _HAPR_SERIAL_H

int tty_read_blocking(char *buf,int length);
int tty_write_blocking(char *buf,int length);
void tty_write(char *buf);
void tty_writeln(char *buf);
void tty_write_binaryten(uint32_t n);
void tty_write_binary(int n);
void tty_write_int(int n);
void tty_writeln_int(int n);
void serial_init();


#endif