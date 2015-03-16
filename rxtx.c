#include "utils.h"
//#include <pthread.h>

#define LOCAL_PORT 1090
#define DPC_PORT   11235
#define DPC_ADDR   "192.168.1.130"
#define THRESHOLD  2048
#define MAXBUF     4096

static unsigned char buffer[MAXBUF];
static const char hb_buf[] = "This is heartbeat frame.\n";

void load_profile(void)
{
    //
}

void watch_dog(void)
{
    //
}


int main(int argc, char const *argv[])
{
    int tty, dpc_sk, local_sk;
    int ret;
    int baudrate = 115200;
    struct pollfd fdset[2];

    if (argc == 2)
        baudrate = atoi(argv[1]);

    //load_profile();

    local_sk = create_local_sk(LOCAL_PORT);

    tty = uart_open(baudrate);
    if (tty == -1) {
        /*
         * sometimes we should initiatively do reset and log error information
         * if we know what happens
         */
        if (send(local_sk, &ERROPENTTY, 1, MSG_DONTWAIT) <= 0)
            perror("rxtx - send ERROPENTTY to main error");
        exit(EXIT_FAILURE);
    }

    dpc_sk = create_dpc_sk(DPC_ADDR, DPC_PORT);
    if (dpc_sk == -1) {
        if (send(local_sk, &ERRCONNECTDPC, 1, MSG_DONTWAIT) <= 0)
            perror("rxtx - send ERRCONNECTDPC to main error");
        exit(EXIT_FAILURE);
    }

    fdset[0].fd     = tty;
    fdset[0].events = POLLIN;

    fdset[1].fd     = dpc_sk;
    fdset[1].events = POLLIN;

    //how to know the socket is disconnected in this loop
    for (;;) {
        fdset[0].revents = 0;
        fdset[1].revents = 0;
        switch (poll(fdset, 2, 10000)) {
            case -1:
                err_exit("rxtx - poll");
            case 0:
                puts("rxtx - no tty data, send heartbeat data");
                if (send(dpc_sk, hb_buf, 25, MSG_DONTWAIT) < 0)
                    err_exit("rxtx - send heartbeat");
                break;
            default:
                if (fdset[0].revents) {
                    ret = read(tty, buffer, MAXBUF);
                    if (ret <= 0) {
                        perror("rxtx - read tty");
                        break;
                    }

                    if (send(dpc_sk, buffer, ret, MSG_DONTWAIT) < 0) {
                        err_exit("rxtx - tcp send");
                        //try again, then tell parent
                    }
                }
                if (fdset[1].revents) {
                    //receive dpc configure data
                }
        }
    }

    exit(0);
}
