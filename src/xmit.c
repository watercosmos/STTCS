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

    char frame1[20] = { 0x7e, 0x00, 0x09, 0x00, 0x01,
                        0x00, 0x00, 0x01, 0x2C, 0x80,
                        0x42, 0xD4, 0x9C, 0x00, 0x55,
                        0x00, 0x5F, 0x8A, 0x67, 0x5A };
    char frame2[20] = { 0x7e, 0x00, 0x09, 0x00, 0x02,
                        0x00, 0x00, 0x01, 0x2C, 0x80,
                        0x00, 0xD4, 0x9C, 0x00, 0x55,
                        0x00, 0x5F, 0xF8, 0xCD, 0x5A };

    port = atoi(argv[2]);

    if ((dpc_sk = create_dpc_sk(argv[1], port)) == -1)
        puts_exit("... create socket error");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 5) {
                if (send(dpc_sk, frame2, 20, 0) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (send(dpc_sk, frame1, 20, 0) > 0)
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

    char frame1[20] = { 0x7e, 0x00, 0x09, 0x00, 0x01,
                        0x00, 0x00, 0x01, 0x2C, 0x80,
                        0x42, 0xD4, 0x9C, 0x00, 0x55,
                        0x00, 0x5F, 0x8A, 0x67, 0x5A };
    char frame2[20] = { 0x7e, 0x00, 0x09, 0x00, 0x02,
                        0x00, 0x00, 0x01, 0x2C, 0x80,
                        0x00, 0xD4, 0x9C, 0x00, 0x55,
                        0x00, 0x5F, 0xF8, 0xCD, 0x5A };
    char frame3[17] = { 0x7e, 0x00, 0x06, 0x00, 0x01,
                        0x00, 0x00, 0x01, 0x2c, 0x80,
                        0x42, 0xd4, 0x9c, 0x01, 0x9f,
                        0x2a, 0x5a };
    char frame4[15] = { 0x7e, 0x00, 0x04, 0x00, 0x01,
                        0x00, 0x00, 0x01, 0x00, 0x55,
                        0x00, 0x5f, 0x8a, 0x9c, 0x5a };

    printf("... xmit: baudrate is %d\n", baudrate);

    tty = uart_open(baudrate);
    if (tty == -1)
        puts_exit("... xmit: can't open tty");

    for (;;) {
        sleep(1);
        color++;

        for (n = 1; n <= 4; n <<= 1) {
            if (color == 5) {
                if (write(tty, frame1, 20) > 0) {
                    color = 0;
                    break;
                }
            } else {
                if (write(tty, frame2, 20) > 0)
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
