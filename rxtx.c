#include "utils.h"
#include <pthread.h>

#define DPC_PORT   11235
#define DPC_ADDR   "192.168.1.130"
#define THRESHOLD  2048
#define MAXBUF     4096

static unsigned char buffer[MAXBUF];
static unsigned char dpc_buf[MAXBUF];
static const char HB_BUF[] = "This is heartbeat frame.\n";
#define HB_LEN 25

static void send_err(int sock, char ERR)
{
    if (send(sock, &ERR, 1, 0) <= 0)
        perror("rxtx - send ERR to main%s\n");
    exit(EXIT_FAILURE);
}

//mutux problem ?
static void * thread_read_dpc(void *arg)
{
    int ret, dpc, local;
    struct pollfd dpcset[1];

    dpc = *(int *)arg;
    local = *((int *)arg + 1);

    dpcset[0].fd      = dpc;
    dpcset[0].events  = POLLIN;
    dpcset[0].revents = 0;

    for (;;) {
        dpcset[0].revents = 0;

        switch (poll(dpcset, 1, 10000)) {
            case -1:
                err_exit("rxtx - pthread poll");
            case 0:
                puts("lost DPC connection");
                send_err(local, ENETWORK);
            default:
                if (dpcset[0].revents != POLLIN) {
                    //something is wrong
                    //maybe we lost network
                }

                //could once rend() receive a whole request frame ?
                ret = read(dpc, dpc_buf, MAXBUF);
                if (ret == -1)
                    err_exit("rxtx - thread read");
                else if (ret == 0) {
                    puts("DPC is orderly disconnected");
                    send_err(local, EDPCCLOSE);
                } else if (ret == 1) {
                    if (dpc_buf[0] == DPCHEARTBEAT) {
                        //connection with dpc is good
                        break;
                    }
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

static void send_retry(int dsk, const char *buf, int buflen, int lsk)
{
    int n;

    for (n = 1; n <= 4; n <<= 1) {
        if (send(dsk, buf, buflen, 0) > 0)
            return;

        perror("rxtx - send to DPC failed");
        if (n <= 4 / 2) {
            puts("rxtx - retry");
            sleep(n);
        }
    }

    send_err(lsk, ENETWORK);
}

int main(int argc, char const *argv[])
{
    int tty, local_sk, dpc_sk;
    int arg[2];
    int ret;
    pthread_t tid;
    struct pollfd fdset[1];
    int baudrate = (argc == 2) ? atoi(argv[1]) : DEFAULTBAUD;

    local_sk = create_local_sk();

    if ((tty = uart_open(baudrate)) == -1)
        send_err(local_sk, EOPENTTY);

    if ((dpc_sk = create_dpc_sk(DPC_ADDR, DPC_PORT)) == -1)
        send_err(local_sk, ECONNECTDPC);

    arg[0] = dpc_sk;
    arg[1] = local_sk;

    if (pthread_create(&tid, NULL, thread_read_dpc, (void *)arg)) {
        puts("rxtx- can't create thread");
        exit(EXIT_FAILURE);
    }

    fdset[0].fd     = tty;
    fdset[0].events = POLLIN;

    //if DPC is down, it takes about 15 minutes to detect without keepalive.
    for (;;) {
        fdset[0].revents = 0;

        switch (poll(fdset, 1, 10000)) {
            case -1:
                err_exit("rxtx - poll");
            case 0:
                puts("rxtx - no tty data, send heartbeat data");
                send_retry(dpc_sk, HB_BUF, HB_LEN, local_sk);
                break;
            default:
                if ((ret = read(tty, buffer, MAXBUF)) <= 0) {
                    perror("rxtx - read tty data");
                    break;
                }
                send_retry(dpc_sk, buffer, ret, local_sk);
        }
    }

    exit(0);
}
