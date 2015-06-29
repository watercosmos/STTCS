#include "utils.h"

int main(int argc, char const *argv[])
{
    //this part is used for sending to DPC
    /*if (argc != 3) {
        puts("usage: xmit [IP] [PORT]");
        exit(1);
    }

    int dpc_sk;
    int port, rlen, n;
    int color = 0;

    char frame1[31] = { 0x7e, 0x00, 0x1f, 0x00, 0x01,
                        0x00, 0x03, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0xff, 0xff,
                        0x5a };
    char frame2[31] = { 0x7e, 0x00, 0x1f, 0x00, 0x01,
                        0x00, 0x03, 0x00, 0x00, 0x80,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0xff, 0xff,
                        0x5a };

    port = atoi(argv[2]);

    if ((dpc_sk = create_dpc_sk(argv[1], port)) == -1)
        puts_exit("... create socket error");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 5) {
                if (send(dpc_sk, frame2, 31, 0) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (send(dpc_sk, frame1, 31, 0) > 0)
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
    int baudrate = (argc == 2) ? atoi(argv[1]) : 3500000;

    char frame1[31] = { 0x7e, 0x00, 0x1f, 0x00, 0x01,
                        0x00, 0x03, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0xff, 0xff,
                        0x5a };
    char frame2[31] = { 0x7e, 0x00, 0x1f, 0x00, 0x01,
                        0x00, 0x03, 0x00, 0x00, 0x80,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0xff, 0xff,
                        0x5a };

    printf("... xmit: baudrate is %d\n", baudrate);

    tty = uart_open(baudrate);
    if (tty == -1)
        puts_exit("... xmit: can't open tty");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 5) {
                if (write(tty, frame2, 31) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (write(tty, frame1, 31) > 0)
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
