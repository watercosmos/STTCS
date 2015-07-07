#include "utils.h"

int main(int argc, char const *argv[])
{
    char frame11[20] = { 0x7E, 0x00, 0x09, 0x00, 0x01,
                         0x00, 0x00, 0x01, 0x2C, 0x80,
                         0x42, 0xD4, 0x9C, 0x00, 0x55,
                         0x00, 0x5F, 0x8A, 0x67, 0x5A };
    char frame12[20] = { 0x7E, 0x00, 0x09, 0x00, 0x01,
                         0x00, 0x00, 0x01, 0x55, 0x80,
                         0x00, 0xD5, 0x9C, 0x00, 0x44,
                         0x00, 0x5E, 0x7F, 0xBD, 0x5A };
    char frame13[20] = { 0x7E, 0x00, 0x09, 0x00, 0x01,
                         0x00, 0x00, 0x01, 0x55, 0x80,
                         0x10, 0xD5, 0x9C, 0x00, 0x45,
                         0x00, 0x5E, 0x7F, 0xF6, 0x5A };
    char frame21[20] = { 0x7E, 0x00, 0x09, 0x00, 0x02,
                         0x00, 0x00, 0x01, 0x2C, 0x80,
                         0x00, 0xD4, 0x9C, 0x00, 0x55,
                         0x00, 0x5F, 0xF8, 0xCD, 0x5A };
    char frame22[20] = { 0x7E, 0x00, 0x09, 0x00, 0x02,
                         0x00, 0x00, 0x01, 0x55, 0x80,
                         0x10, 0x35, 0x9C, 0x80, 0x65,
                         0x00, 0x6E, 0x56, 0xC6, 0x5A };
    char frame23[20] = { 0x7E, 0x00, 0x09, 0x00, 0x02,
                         0x00, 0x00, 0x01, 0x55, 0x50,
                         0x10, 0x25, 0x9C, 0x80, 0x65,
                         0x80, 0x1E, 0x1F, 0xA8, 0x5A };

    //this part is used for sending to DPC
    /*if (argc != 3) {
        puts("usage: xmit [IP] [PORT]");
        exit(1);
    }

    int dpc_sk;
    int port, rlen, n;
    int color = 0;

    port = atoi(argv[2]);

    if ((dpc_sk = create_dpc_sk(argv[1], port)) == -1)
        puts_exit("... create socket error");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 3) {
                if (send(dpc_sk, frame23, 20, 0) > 0)
                    break;
            } else if (color == 5) {
                if (send(dpc_sk, frame12, 20, 0) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (send(dpc_sk, frame13, 20, 0) > 0)
                    break;
            }

            perror("... send to DPC failed");
            if (n <= 4 / 2) {
                puts("... retry");
                sleep(n);
            }
        }
    }*/

    //this part is used for sending from uart
    int tty, rlen, n;
    int color = 0;
    int baudrate = (argc == 2) ? atoi(argv[1]) : 115200;

    printf("... xmit: baudrate is %d\n", baudrate);

    tty = uart_open(baudrate);
    if (tty == -1)
        puts_exit("... xmit: can't open tty");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 3) {
                if (write(tty, frame11, 20) > 0)
                    break;
            } else if (color == 5) {
                if (write(tty, frame12, 20) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (write(tty, frame13, 20) > 0)
                    break;
            }

            perror("... send failed");
            if (n <= 4 / 2) {
                puts("... retry");
                sleep(n);
            }
        }
    }

    exit(0);
}
