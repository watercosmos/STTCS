#include "utils.h"

#define MAXBUF 4096
#define DEFAULTBAUD 3500000

int main(int argc, char const *argv[])
{
    int tty;
    FILE *out;
    char buffer[MAXBUF];
    int baudrate = DEFAULTBAUD;

    if (argc == 2)
        baudrate = atoi(argv[1]);
    tty = uart_open(baudrate);
    if (tty == -1)
        exit(EXIT_FAILURE);

    if ((out = fdopen(tty, "a")) == NULL)
        err_exit("file open");

    for (;;) {
        if (fgets(buffer, MAXBUF, stdin) == NULL)
            err_exit("read done or error");

        if (fputs(buffer, out) == EOF)
            err_exit("write error");
    }

    exit(0);
}
