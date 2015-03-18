
#include "utils.h"
#include <signal.h>

#define MAXBUF 32

static unsigned char buffer[MAXBUF] = { '\0' };

static void load_profile(void)
{
    //read ./.config
    //assemble the parameters for rxtx
}

/*
 * create a server socket in order to connect with "rxtx" child process
 * the socket is only used for error report by "rxtx"
 * just listen(), doesn't accept(), available for only one connection
 * return socket descriptor for accept(), on error, exit process
 */
static int init_sock(void)
{
    int sk;
    int reuse = 1;
    struct sockaddr_in addr;

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("main - socket");

    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
        err_exit("main - setsockopt");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(LOCAL_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&addr,
        sizeof(struct sockaddr)) == -1)
        err_exit("main - bind");
    if (listen(sk, 1) == -1)
        err_exit("main - listen");

    return sk;
}

static void check_network(void)
{
    //exponential backoff
    //if network is not available, exit
}

/*
 * launch "rxtx" child process, return process id
 */
static pid_t launch_rxtx(const char *baudrate)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        err_exit("main - fork error");
    else if (pid == 0) {
        printf("rxtx - start, pid is %ld\n", (long)getpid());
        if (execl("./rxtx", "rxtx", baudrate, (char *)NULL) == -1)
            err_exit("rxtx - execl error");
    }

    return pid;
}

/*
 * establish the connection with "rxtx" child process
 * if it can't be done in 10 seconds, return -1
 * otherwise return the socket descriptor
 */
static int accept_child(int sock)
{
    int child_sk;
    struct pollfd fdset[1];

    fdset[0].fd     = sock;
    fdset[0].events = POLLIN;

    switch (poll(fdset, 1, 10000)) {
        case -1:
            err_exit("main - poll");
        case 0:
            puts("main - accept rxtx timeout");
            return -1;
        default:
            if ((child_sk = accept(sock, NULL, NULL)) == -1)
                err_exit("main - accept");

            puts("main - rxtx is connected");

            return child_sk;
    }
}

/*
 * detect child status and socket receiving
 * watchdog if nothing is wrong
 * return -1 if any error report is received
 */
static int serve(int sock)
{
    struct pollfd fdset[1];

    fdset[0].fd     = sock;
    fdset[0].events = POLLIN;

    memset(buffer, '\0', MAXBUF);   //buffer will be used only once.

    for (;;) {
        switch (waitpid(-1, NULL, WNOHANG)) {
            case -1:
                err_exit("main - no child or interrupt by signal");
            case 0:
                break;
            default:
                //maybe return -1 is better ?
                err_exit("main - rxtx unexpectedly died");
        }

        fdset[0].revents = 0;

        //normally, nothing will be received
        //just watchdog when poll() time out
        switch (poll(fdset, 1, 30000)) {
            case -1:
                err_exit("main - poll");
            case 0:
                //watch dog here
                puts("main - everything is fine");
                break;
            default:
                //child will never orderly disconnect
                if (read(sock, buffer, MAXBUF) <= 0)
                    err_exit("main - read");

                //logfile should be recorded
                switch (buffer[0]) {
                    case EOPENTTY:
                        puts("main - EOPENTTY: rxtx open tty error");
                        return -1;
                    case ECONNECTDPC:
                        puts("main - ECONNECTDPC: rxtx can't connect to DPC");
                        return -1;
                    case ENETWORK:
                        puts("main - ENETWORK: DPC is unreachable");
                        return -1;
                    case EDPCCLOSE:
                        puts("main - ENETWORK: DPC initiatively closed");
                        return -1;
                    default:
                        printf("main - %s\n", buffer);
                        return -1;
                }
        }
    }
}

/*
 * release the zombie child process
 * if the child is still alive, kill it before release it
 * make sure there is a child exist before we call this function
 * on error, exit process
 */
static void clean_child(pid_t pid)
{
    switch (waitpid(-1, NULL, WNOHANG)) {
        case -1:
            err_exit("main - no child or interrupt by signal");
        case 0:
            puts("main - child is still alive, kill it");
            if (kill(pid, SIGKILL) == -1)
                err_exit("main - kill");
            if (wait(NULL) != pid)  //is it possible to block ?
                err_exit("main wait");
        default:
            puts("main - clean child done");
    }
}

int main(int argc, char const *argv[])
{
    int sk;
    //const char *baudrate = (argc == 2) ? argv[1] : "DEFAULTBAUD";
    const char *baudrate = "3500000";

    if (argc == 2)
        baudrate = argv[1];

    sk = init_sock();

    for (;;) {
        pid_t pid;
        int   child_sk;

        check_network();

        pid = launch_rxtx(baudrate);

        if ((child_sk = accept_child(sk)) == -1) {
            clean_child(pid);
            puts("main - restart rxtx");
            continue;
        }

        if (serve(child_sk) == -1) {
            close(child_sk);
            sleep(1);    //make sure child is dead.
            clean_child(pid);

            puts("main - restart rxtx");

            continue;
        }
    }

    exit(0);
}
