#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DEFAULTBAUD 3500000
#define MAXBUF      4096
#define LOCAL_PORT  1090
#define DPC_PORT    11235
//#define DPC_ADDR    "192.168.1.130"
#define DPC_ADDR    "192.168.43.1"

/*
 **************** protocol with main application ****************
 */

#define EOPENTTY    '1'
#define ECONNECTDPC '2'
#define ENETWORK    '3'
#define EDPCCLOSE   '4'

#define DPCHEARTBEAT '9'


/*
 ***************************** tool *****************************
 */

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define puts_exit(msg) \
    do { puts(msg); exit(EXIT_FAILURE); } while (0)

/*
 ***************************** GPIO *****************************
 */

#define NEO_GPIO_DEVIO 'N'

#define NEO_IOCTL_GPIO_SET_VALUE    _IOW(NEO_GPIO_DEVIO,1,char[3])
#define NEO_IOCTL_GPIO_GET_VALUE    _IOR(NEO_GPIO_DEVIO,2,char[3])
#define NEO_IOCTL_GPIO_SET_DIR      _IOW(NEO_GPIO_DEVIO,3,char[3])
#define NEO_IOCTL_GPIO_GET_DIR      _IOR(NEO_GPIO_DEVIO,4,char[3])

#define NEO_IOCTL_GPIO_MAXNR 4

int  gpio[15] = {6,   35,  102, 117, 186,
                 187, 188, 189, 190, 199,
                 200, 211, 212, 213, 214};
/*
 * input:  dir = 0
 * output: dir = 1
 */
static int neoGpioSetDir(int gpio_n, int dir, int value)
{
    int fd     = 0;
    int result = -2;
    unsigned int cmd;
    char arg[3];

    fd = open("/dev/neo_gpio_dev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    cmd    = NEO_IOCTL_GPIO_SET_DIR;
    arg[0] = gpio_n;
    arg[1] = dir;
    arg[2] = value;

    if (ioctl(fd, cmd, &arg) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    result = arg[1];
    close(fd);
    return result;
}

static int neoGpioGetDir(int gpio_n)
{
    int fd     = 0;
    int result = -1;
    unsigned int cmd;
    char arg[3];

    fd = open("/dev/neo_gpio_dev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    cmd    = NEO_IOCTL_GPIO_GET_DIR;
    arg[0] = gpio_n;

    if (ioctl(fd, cmd, &arg) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    result = arg[1];
    close(fd);
    return result;
}

static int neoGpioSetValue(int gpio_n, int value)
{
    int fd = 0;
    unsigned int cmd;
    char arg[3];

    fd = open("/dev/neo_gpio_dev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    cmd    = NEO_IOCTL_GPIO_SET_VALUE;
    arg[0] = gpio_n;
    arg[1] = value;

    if (ioctl(fd, cmd, &arg) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

static int neoGpioGetValue(int gpio_n)
{
    int fd     = 0;
    int result = -2;
    unsigned int cmd;
    char arg[3];

    fd = open("/dev/neo_gpio_dev", O_RDWR);
    if (fd < 0) {
        perror("ioctl");
        return -1;
    }

    cmd    = NEO_IOCTL_GPIO_GET_VALUE;
    arg[0] = gpio_n;

    if (ioctl(fd, cmd, &arg) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    result = arg[1];
    close(fd);

    return result;
}

void setAllUp(void)
{
    int i, j;
    j = sizeof(gpio) / sizeof(int);
    for (i = 0; i < j; i++) {
        if (neoGpioSetDir(gpio[i], 1, 1) == -1)
            fprintf(stderr, "SetDir   %d: %s\n", gpio[i], strerror(errno));
        else if (neoGpioSetValue(gpio[i], 1) == -1)
            fprintf(stderr, "SetValue %d: %s\n", gpio[i], strerror(errno));
    }
}

void setAllDown(void)
{
    int i, j;
    j = sizeof(gpio) / sizeof(int);
    for (i = 0; i < j; i++) {
        if (neoGpioSetDir(gpio[i], 1, 0) == -1)
            fprintf(stderr, "SetDir   %d: %s\n", gpio[i], strerror(errno));
        else if (neoGpioSetValue(gpio[i], 0) == -1)
            fprintf(stderr, "SetValue %d: %s\n", gpio[i], strerror(errno));
    }
}


/*
 ***************************** UART *****************************
 */

static speed_t getBaudrate(int baudrate)
{
    switch (baudrate) {
        case 0:       return B0;
        case 50:      return B50;
        case 75:      return B75;
        case 110:     return B110;
        case 134:     return B134;
        case 150:     return B150;
        case 200:     return B200;
        case 300:     return B300;
        case 600:     return B600;
        case 1200:    return B1200;
        case 1800:    return B1800;
        case 2400:    return B2400;
        case 4800:    return B4800;
        case 9600:    return B9600;
        case 19200:   return B19200;
        case 38400:   return B38400;
        case 57600:   return B57600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        case 460800:  return B460800;
        case 500000:  return B500000;
        case 576000:  return B576000;
        case 921600:  return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 1500000: return B1500000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 3500000: return B3500000;
        case 4000000: return B4000000;
        default:      return -1;
    }
}

int uart_open(int baudrate)
{
    int fd;
    speed_t speed;
    struct termios cfg;

    if ((speed = getBaudrate(baudrate)) == -1) {
        fprintf(stderr, "Invalid baudrate\n");
        return -1;
    }

    if ((fd = open("/dev/ttyHSL0", O_RDWR | 0)) == -1) {
        perror("Cannot open ttyHSL0");
        return -1;
    }

    if (tcgetattr(fd, &cfg)) {
        fprintf(stderr, "tcgetattr() failed\n");
        close(fd);
        return -1;
    }

    cfmakeraw(&cfg);
    cfsetispeed(&cfg, speed);
    cfsetospeed(&cfg, speed);

    if (tcsetattr(fd, TCSANOW, &cfg)) {
        fprintf(stderr, "tcsetattr() failed\n");
        close(fd);
        return -1;
    }

    return fd;
}


/*
 ***************************** TCP *****************************
 */

static void set_nonblock(int sock)
{
    int opts;

    if ((opts = fcntl(sock, F_GETFL)) == -1)
        err_exit("... rxtx: fcntl F_GETFL");

    opts |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, opts) == -1)
        err_exit("... rxtx: fcntl F_SETFL");
}

static int connect_retry(int sock, const struct sockaddr *addr)
{
    int n;

    for (n = 1; n <= 4; n <<= 1) {
        if (connect(sock, addr, sizeof(struct sockaddr)) == 0) {
            puts("... rxtx: connected to DPC");
            return 0;
        }
        perror("... rxtx: connet to DPC failed");
        if (n <= 4 / 2) {
            puts("... rxtx: retry");
            sleep(n);
        }
    }

    puts("... rxtx: give up");

    return -1;
}

int create_dpc_sk(char *ip, uint16_t port)
{
    int sk;
    int count = 0;
    struct sockaddr_in addr;

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("... rxtx: dpc socket");

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip, &addr.sin_addr) == 0)
        puts_exit("... rxtx: invalid DPC ip address");

    if (connect_retry(sk, (struct sockaddr *)&addr) == -1)
        return -1;

    set_nonblock(sk);

    return sk;
}

int create_local_sk(void)
{
    int sk;
    struct sockaddr_in addr;

    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("... rxtx: local socket");

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LOCAL_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(sk, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1)
        err_exit("... rxtx: local connect");

    set_nonblock(sk);

    return sk;
}

void send_err(int sock, char ERR)
{
    if (send(sock, &ERR, 1, 0) <= 0)
        perror("... rxtx: send ERR to main%s\n");
    exit(EXIT_FAILURE);
}

void send_retry(int dsk, const char *buf, int buflen, int lsk)
{
    int n;

    for (n = 1; n <= 4; n <<= 1) {
        if (send(dsk, buf, buflen, 0) > 0)
            return;

        perror("... rxtx: send to DPC failed");
        if (n <= 4 / 2) {
            puts("... rxtx: retry");
            sleep(n);
        }
    }

    send_err(lsk, ENETWORK);
}
