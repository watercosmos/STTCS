#include "utils.h"
#include <pthread.h>

#define HB_LEN 25
static const char HB_BUF[] = "This is heartbeat frame.\n";

static char buffer[MAXBUF];
static char dpc_buf[MAXBUF];

//mutex problem ?
static void * thread_read_dpc(void *arg)
{
    int ret, dpc, local;
    struct pollfd dpcset[1];

    dpc   = *(int *)arg;
    local = *((int *)arg + 1);

    dpcset[0].fd      = dpc;
    dpcset[0].events  = POLLIN;
    dpcset[0].revents = 0;

    for (;;) {
        dpcset[0].revents = 0;

        switch (poll(dpcset, 1, 15000)) {
            case -1:
                err_exit("... rxtx: pthread poll");
            case 0:
                puts("... rxtx: lost DPC connection");
                send_err(local, ENETWORK);
            default:
                if (dpcset[0].revents != POLLIN) {
                    //something is wrong
                    //maybe we lost network connectivity
                    puts("... rxtx: someting is wrong");
                }

                //could once rend() receive a whole request frame ?
                ret = read(dpc, dpc_buf, MAXBUF);
                if (ret == -1)
                    err_exit("... rxtx: thread read");
                else if (ret == 0) {
                    puts("... rxtx: DPC is orderly disconnected");
                    send_err(local, EDPCCLOSE);
                } else if (ret == 1) {
                    if (dpc_buf[0] == DPCHEARTBEAT)
                        break;
                    putchar(dpc_buf[0]);
                    putchar('\n');
                } else {
                    //receive config requedt or data
                    //confirm the request
                    //tell main thread to send confirm frame
                    //then receive config data
                    //while dpc waiting for confirm, it still send heartbeat
                }
        }
    }
}

int main(int argc, char const *argv[])
{
    int tty, local_sk, dpc_sk;
    int arg[2];
    int rlen;
    pthread_t tid;
    struct pollfd fdset[1];
    int baudrate = (argc == 2) ? atoi(argv[1]) : DEFAULTBAUD;

    local_sk = create_local_sk();

    printf("... rxtx: baudrate is %d\n", baudrate);

    if ((tty = uart_open(baudrate)) == -1)
        send_err(local_sk, EOPENTTY);

    if ((dpc_sk = create_dpc_sk(DPC_ADDR, DPC_PORT)) == -1)
        send_err(local_sk, ECONNECTDPC);

    arg[0] = dpc_sk;
    arg[1] = local_sk;

    if (pthread_create(&tid, NULL, thread_read_dpc, (void *)arg))
        puts_exit("... rxtx: can't create thread");

    fdset[0].fd     = tty;
    fdset[0].events = POLLIN;

    for (;;) {
        fdset[0].revents = 0;

        switch (poll(fdset, 1, 10000)) {
            case -1:
                err_exit("... rxtx: poll");
            case 0:
                puts("... rxtx: no tty data, send heartbeat data");
                send_retry(dpc_sk, HB_BUF, HB_LEN, local_sk);
                break;
            default:
                if ((rlen = read(tty, buffer, MAXBUF)) <= 0) {
                    perror("... rxtx: read tty data");
                    break;
                }
                send_retry(dpc_sk, buffer, rlen, local_sk);
        }
    }

    exit(0);
}
