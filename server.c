#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#define MAX_BUF 4096
#define PORT    11235
#define ADDR    "192.168.1.130"
#define DPCHEARTBEAT '9'

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static int send_retry(int dsk, char buf, int buflen)
{
    int n;

    for (n = 1; n <= 4; n <<= 1) {
        if (send(dsk, &buf, buflen, MSG_DONTWAIT) > 0)
            return 0;

        perror("send heartbeat to client failed");

        if (n <= 4 / 2) {
            puts("retry");
            sleep(n);
        }
    }

    return -1;
}

int main(int argc, char *argv[])
{
    int sk, client_sk, fd;
    int rlen;
    int flag  = 0;
    int reuse = 1;
    char buffer[MAX_BUF];
    struct pollfd fdset[1];
    socklen_t client_len;
    struct sockaddr_in local_addr, client_addr;

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("socket");

    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
        err_exit("setsockopt");

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons(PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&local_addr,
        sizeof(struct sockaddr)) == -1)
        err_exit("bind");

    if (listen(sk, 1) == -1)
        err_exit("listen");

    puts("start server");

DISCONNECT:
    if (flag) {
        close(fd);
        close(client_sk);
        puts("disconnect");
    }

    for (;;) {
        client_len = sizeof(struct sockaddr_in);
        memset(&client_addr, 0 ,sizeof(client_addr));

        client_sk = accept(sk, (struct sockaddr *)&client_addr, &client_len);
        if (client_sk == -1)
            err_exit("accept");

        printf("connected\nfrom: %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

        fd = open("recvfile", O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (fd == -1)
            err_exit("recvfile open error");

        flag = 1;

        fdset[0].fd      = client_sk;
        fdset[0].events  = POLLIN;
        fdset[0].revents = 0;

        for (;;) {
            fdset[0].revents = 0;

            switch (poll(fdset, 1, 5000)) {
                case -1:
                    err_exit("poll");
                case 0:
                    if (send_retry(client_sk, DPCHEARTBEAT, 1))
                        goto DISCONNECT;
                    break;
                default:
                    if ((rlen = recv(client_sk, buffer, MAX_BUF, 0)) <= 0) {
                        perror("recv");
                        goto DISCONNECT;
                    }

                    if (write(fd, buffer, rlen) != rlen)
                        perror("write");

                    if (send_retry(client_sk, DPCHEARTBEAT, 1))
                        goto DISCONNECT;
            }
        }
    }

    exit(0);
}
