
#include "utils.h"

#define MAXBUF 32
#define LOCAL_PORT 1090

static unsigned char buffer[MAXBUF] = { '\0' };

int main(int argc, char const *argv[])
{
    int sk, child_sk;
    pid_t pid, ret;
    struct sockaddr_in addr;
    const char *baudrate = "115200";

    if (argc == 2)
        baudrate = argv[1];

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("main - socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(LOCAL_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&addr,
        sizeof(struct sockaddr)) == -1)
        err_exit("main - bind");
    if (listen(sk, 1) == -1)
        err_exit("main - listen");

    if ((pid = fork()) < 0)
        err_exit("main - fork error");
    else if (pid == 0) {
        printf("rxtx - pid = %ld\n", (long)getpid());
        if (execl("./rxtx", "rxtx", baudrate, (char *)NULL) == -1)
            err_exit("rxtx - execl error");
    }

    if ((child_sk = accept(sk, NULL, NULL)) == -1)
        err_exit("accept");

    puts("main - rxtx process is connected");

    struct pollfd fdset[1];

    fdset[0].fd     = child_sk;
    fdset[0].events = POLLIN;

    for (;;) {
        switch (waitpid(-1, NULL, WNOHANG)) {
            case -1:
                err_exit("main - waitpid");
            case 0:
                break;
            default:
                puts("main - rxtx is dead");
                exit(EXIT_FAILURE);
        }

        fdset[0].revents = 0;
        memset(buffer, '\0', MAXBUF);

        switch (poll(fdset, 1, 30000)) {
            case -1:
                err_exit("main - poll");
            case 0:
                puts("main - everything is fine");
                break;
            default:
                if (fdset[0].revents == 0)
                    puts("main - strange I/O");
                else if (fdset[0].revents & POLLHUP)
                    err_exit("main - child is disconnected");

                if (read(child_sk, buffer, MAXBUF) < 0)
                    err_exit("main - read");

                switch (buffer[0]) {
                    case ISEOPENTTY:
                        puts("main - EOPENTTY: rxtx open tty error");
                        ret = wait(NULL);
                        if (ret <= 0)
                            err_exit("main - wait");
                        else if (ret != pid)
                            puts("main - return pid != child pid");
                        exit(EXIT_FAILURE);
                    case ISECONNECTDPC:
                        printf("main - recv ECONNECTDPC: ");
                        printf("rxtx can't connect to DPC\n");
                        ret = wait(NULL);
                        if (ret <= 0)
                            err_exit("main - wait");
                        else if (ret != pid)
                            puts("main - return pid != child pid");
                        exit(EXIT_FAILURE);
                    case ISENETWORK:
                        //check available network here
                        //restart rxtx child process
                        //and continue listening msg from rxtx
                    default:
                        printf("main - %s\n", buffer);
                }
        }
    }

    exit(0);
}
