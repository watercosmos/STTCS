#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_BUF 4096
#define PORT    11235
#define ADDR    "192.168.1.130"

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
    int sk;
    struct sockaddr_in local_addr;

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("socket");

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
    for (;;) {
        int fd;
        int client_sk;
        socklen_t client_len;
        struct sockaddr_in client_addr;

        memset(&client_addr, 0 ,sizeof(client_addr));
        client_len = sizeof(struct sockaddr_in);

        client_sk = accept(sk, (struct sockaddr *)&client_addr, &client_len);
        if (client_sk == -1)
            err_exit("accept");

        printf("connected\nfrom: %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

        fd = open("recvfile", O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (fd == -1)
            err_exit("recvfile open error");

        char recv_buf[MAX_BUF];
        memset(recv_buf, 0, MAX_BUF);

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(client_sk, &read_set);

        struct timeval tv;
        tv.tv_sec  = 20;
        tv.tv_usec = 0;

        for (;;) {
            switch (select(client_sk + 1, &read_set, NULL, NULL, &tv) >= 0) {
                case -1:
                    err_exit("select");
                case 0:
                    tv.tv_sec  = 20;
                    tv.tv_usec = 0;
                    puts("beats");
                    break;
                default:
                    tv.tv_sec  = 20;
                    tv.tv_usec = 0;

                    if (!FD_ISSET(client_sk, &read_set)) {
                        FD_ZERO(&read_set);
                        FD_SET(client_sk, &read_set);
                        puts("oops");
                        continue;
                    }

                    FD_ZERO(&read_set);
                    FD_SET(client_sk, &read_set);

                    int rlen = recv(client_sk, recv_buf, MAX_BUF, 0);
                    if (rlen <= 0) {
                        perror("recv");
                        puts("disconnect");
                        close(client_sk);
                        goto DISCONNECT;
                        break;
                    }
                    if (write(fd, recv_buf, rlen) != rlen)
                        perror("write");
            }
        }
    }
}
