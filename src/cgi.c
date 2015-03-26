#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define puts_exit(msg) \
    do { puts(msg); exit(EXIT_FAILURE); } while (0)

extern char **environ;

int main(int argc, char const *argv[])
{
    char *env;
    char ip[16];
    int port, baudrate;
    FILE *fp;
    char buf[32];

    if ((env = getenv("ip")) == NULL)
        puts_exit("no ip");
    strncpy(ip, env, 16);

    if ((env = getenv("port")) == NULL)
        puts_exit("no port");
    port = atoi(env);

    if ((env = getenv("baudrate")) == NULL)
        puts_exit("no baudrate");
    baudrate = atoi(env);

    if ((fp = fopen("../.config", "w")) == NULL)
        err_exit("open .config");

    snprintf(buf, 25, "DPC_IP = %s", ip);
    fputs(buf, fp);
    fputc('\n', fp);

    snprintf(buf, 17, "DPC_PORT = %d", port);
    fputs(buf, fp);
    fputc('\n', fp);

    snprintf(buf, 19, "BAUDRATE = %d", baudrate);
    fputs(buf, fp);
    fputc('\n', fp);

    return 0;
}
