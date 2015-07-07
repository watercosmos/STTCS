#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>

#define MAXBUF  32
#define MAXLINE 256

//network operation
#define WIFION       1
#define WIFIOFF      2
#define MOBILEON     3
#define MOBILEOFF    4
#define CONNECTIVITY 5
#define WIFICHECK    6
#define APMODE       7

//error message from "rxtx" child process
#define EOPENTTY    '1'
#define ECONNECTDPC '2'
#define ENETWORK    '3'
#define EDPCCLOSE   '4'

#define LOCAL_PORT  1090

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define puts_exit(msg) \
    do { puts(msg); exit(EXIT_FAILURE); } while (0)

//default configuration
static char IP[16]   = "104.224.158.114";
static int  PORT     = 11235;
static int  BAUDRATE = 115200;

static int is_comment(char *str)
{
    for (; *str != '\0' && isblank(*str); ++str);

    if (str[0] == '#' || isblank(str[0]) || str[0] == '\n')
        return 1;
    return 0;
}

static char * del_both_trim(char * str)
{
    char *ch;

    for (; *str != '\0' && isblank(*str); ++str);

    for (ch = str + strlen(str) - 1;
         ch >= str && (isblank(*ch) || *ch == '\n'); --ch);

    *(++ch) = '\0';

    return str;
}

static void read_cfg(void)
{
    FILE *fp;
    char buf[64];
    char *delim = "=";
    char *item, *key, *value;

    if ((fp = fopen("/data/src/.config", "r")) == NULL) {
        if (errno == ENOENT) {
            puts("... main: no \"/data/src/.config\", use default parameters");
            return;
        } else
            err_exit("... main: open /data/src/.config error");
    }

    //make sure once .config is exist, it is never a blank file
    while ((item = fgets(buf, sizeof(buf), fp)) != NULL) {
        if (is_comment(item))
            continue;

        key = del_both_trim(strtok(buf, delim));
        value = del_both_trim(strtok(NULL, delim));

        if (!value)
            puts_exit("... main: config file syntax error");

        if (!strncmp(key, "DPC_IP", 6))
            strcpy(IP, value);
        else if (!strncmp(key, "DPC_PORT", 8))
            PORT = atoi(value);
        else if (!strncmp(key, "BAUDRATE", 8))
            BAUDRATE = atoi(value);
        else
            puts_exit("... main: config file syntax error");
    }

    printf("... main: ip: %s\n          port: %d\n          baudrate: %d\n",
           IP, PORT, BAUDRATE);
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
        err_exit("... main: socket");

    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
        err_exit("... main: setsockopt");

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(LOCAL_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&addr,
        sizeof(struct sockaddr)) == -1)
        err_exit("... main: bind");
    if (listen(sk, 1) == -1)
        err_exit("... main: listen");

    return sk;
}

static int network_manager(int cmd)
{
    int fd[2];
    pid_t pid;
    char line[MAXLINE];
    char arg[4];
    int rlen, offset;

    snprintf(arg, 4, "%d", cmd);

    if (pipe(fd) < 0)
        err_exit("... main: pipe");

    if ((pid = fork()) < 0)
        err_exit("... main: fork check network");
    else if (pid == 0) {
        close(fd[0]);
        if (fd[1] != STDOUT_FILENO) {
            if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
                err_exit("... main: dup2");
            close(fd[1]);
            close(STDERR_FILENO);
        }
        if (execlp("/data/src/net.sh", "net.sh", arg, (char *)0) == -1)
            err_exit("... main: exec sh");
    }

    close(fd[1]);

    rlen = 0;
    offset = 0;
    memset(line, '\0', MAXLINE);

    while ((rlen = read(fd[0], line + offset, MAXLINE)) > 0
           && offset < MAXLINE)
        offset += rlen;
    if (rlen < 0)
        err_exit("... main: read sh result");

    while (waitpid(pid, NULL, 0) <= 0)
        if (errno != EINTR)
            err_exit("... main: wait sh");

    switch (cmd) {
        case WIFION:
        case WIFIOFF:
        case MOBILEON:
        case MOBILEOFF:
            break;
        case CONNECTIVITY:
            if (line[35] == '0') {    //0% packet loss
                puts("... main: network is available");
                break;
            }
            puts("... main: network is unavailable");
            return 0;
        case WIFICHECK:
            if (line[9] == 'e') {
                puts("... main: Wi-Fi is enabled");
                break;
            } else if (line[9] == 'd') {
                puts("... main: Wi-Fi is disabled");
                return 0;
            } else {
                puts("... main: Wi-Fi is strange");
                return 0;
            }
        case APMODE:
            puts("... main: start AP mode");
            break;
        default:
            break;
    }
    return 1;
}

static void check_network(int sk)
{
    for (;;) {
        network_manager(MOBILEON);
        sleep(2);
        network_manager(WIFION);
        sleep(2);

        //should check available ssid

        if (network_manager(CONNECTIVITY))
            return;

        network_manager(WIFIOFF);
        sleep(2);

        if (!network_manager(CONNECTIVITY)) {
            puts("... main: No Internet Access");
            continue;
            //network_manager(APMODE);
            //close(sk);
            //puts("... main: start http server");
            //if (execlp("/data/src/httpd", "httpd", (char *)0) == -1)
            //    err_exit("... main: exec httpd");
            //after configuration, this process should run again
            //exit(EXIT_FAILURE);
        } else
            return;
    }
}

/*
 * launch "rxtx" child process, return process id
 */
static pid_t launch_rxtx(void)
{
    pid_t pid;
    char port[8];
    char baudrate[8];

    snprintf(port, 8, "%d", PORT);
    snprintf(baudrate, 8, "%d", BAUDRATE);

    if ((pid = fork()) < 0)
        err_exit("... main: fork error");
    else if (pid == 0) {
        printf("... rxtx: start, pid is %ld\n", (long)getpid());
        if (execl("/data/src/rxtx", "rxtx", IP, port, baudrate, (char *)NULL) == -1)
            err_exit("... rxtx: execl error");
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
            err_exit("... main: poll");
        case 0:
            puts("... main: accept rxtx timeout");
            return -1;
        default:
            if ((child_sk = accept(sock, NULL, NULL)) == -1)
                err_exit("... main: accept");

            puts("... main: rxtx is connected");

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
    char   buffer[MAXBUF];

    fdset[0].fd     = sock;
    fdset[0].events = POLLIN;

    memset(buffer, '\0', MAXBUF);   //buffer will be used only once.

    for (;;) {
        switch (waitpid(-1, NULL, WNOHANG)) {
            case -1:
                err_exit("... main: no child or interrupt by signal");
            case 0:
                break;
            default:
                //maybe return -1 is better ?
                err_exit("... main: rxtx unexpectedly died");
        }

        fdset[0].revents = 0;

        //normally, nothing will be received
        //just watchdog when poll() time out
        switch (poll(fdset, 1, 30000)) {
            case -1:
                err_exit("... main: poll");
            case 0:
                //watch dog here
                //puts("... main: everything is fine");
                break;
            default:
                //child will never orderly disconnect
                if (read(sock, buffer, MAXBUF) <= 0) {
                    puts("... main: rxtx disconnect without sending ERR");
                    return -1;
                }

                //logfile should be recorded
                switch (buffer[0]) {
                    case EOPENTTY:
                        puts("... main: rxtx open tty error");
                        return -1;
                    case ECONNECTDPC:
                        puts("... main: rxtx can't connect to DPC");
                        return -1;
                    case ENETWORK:
                        puts("... main: DPC is unreachable");
                        return -1;
                    case EDPCCLOSE:
                        puts("... main: DPC initiatively closed");
                        return -1;
                    default:
                        printf("... main: %s\n", buffer);
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
            err_exit("... main: no child or interrupt by signal");
        case 0:
            puts("... main: child is still alive, kill it");
            if (kill(pid, SIGKILL) == -1)
                err_exit("... main: kill");
            if (wait(NULL) != pid)  //is it possible to block ?
                err_exit("... main: wait");
        default:
            puts("... main: clean child done");
    }
}

int main(int argc, char const *argv[])
{
    int sk;

    if (argc == 2)
        BAUDRATE = atoi(argv[1]);

    read_cfg();

    sk = init_sock();

    for (;;) {
        pid_t pid;
        int   child_sk;

        check_network(sk);

        pid = launch_rxtx();

        if ((child_sk = accept_child(sk)) == -1) {
            clean_child(pid);
            puts("... main: restart rxtx");
            continue;
        }

        if (serve(child_sk) == -1) {
            close(child_sk);
            sleep(1);    //make sure child is dead.
            clean_child(pid);

            puts("... main: restart rxtx");

            continue;
        }
    }

    exit(0);
}
