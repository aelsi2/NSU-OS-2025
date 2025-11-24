#include "io.h"
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>

int fdprintf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = vsnprintf(NULL, 0, format, args);
    va_end(args);
    char buffer[count + 1];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    return write(fd, buffer, count);
}

static void print_prompt() {
    char *login = getlogin();
    char hostname[MAXHOSTNAMELEN + 1];
    int hostname_res = gethostname(hostname, sizeof(hostname));
    hostname[MAXHOSTNAMELEN] = 0;
    if (login && !hostname_res) {
        fdprintf(1, "%s@%s > ", login, hostname);
    } else {
        fdprintf(1, "> ");
    }
}

int prompt_line(char *buffer, int len) {
    if (len == 0) {
        return 0;
    }

    print_prompt();

    int line_length = 0;
    while (1) {
        int read_count = read(0, buffer + line_length, len - line_length - 1);
        if (read_count == -1) {
            perror("A read error ocurred");
            return 0;
        }
        line_length += read_count;
        buffer[line_length] = '\0';

        if (line_length >= 2 && buffer[line_length - 2] == '\\' &&
            buffer[line_length - 1] == '\n') {
            buffer[line_length - 2] = ' ';
            line_length -= 1;
            if (read_count) {
                continue;
            }
        }
        return line_length;
    }
}
