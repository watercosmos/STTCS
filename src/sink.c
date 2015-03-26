#include "utils.h"

int main(int argc, char const *argv[])
{
    int tty;
    FILE *out;
    char buffer[MAXBUF];
    int baudrate = (argc == 2) ? atoi(argv[1]) : 3500000;

    printf("... sink: baudrate is %d\n", baudrate);

    tty = uart_open(baudrate);
    if (tty == -1)
        puts_exit("... sink: can't open tty");

    if ((out = fdopen(tty, "a")) == NULL)
        err_exit("... sink: can't open standard i/o stream");

    for (;;) {
        if (fgets(buffer, MAXBUF, stdin) == NULL)
            puts_exit("... sink: send done");

        if (fputs(buffer, out) == EOF)
            err_exit("... sink: write error");
    }

    exit(0);
}
