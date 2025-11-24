#ifndef __IO_H
#define __IO_H

int fdprintf(int fd, const char *format, ...);
int prompt_line(char *buffer, int len);

#endif // __IO_H
