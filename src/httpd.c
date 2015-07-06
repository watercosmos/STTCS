/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>

#define ISspace(x) isspace((int)(x))

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define puts_exit(msg) \
    do { puts(msg); exit(EXIT_FAILURE); } while (0)

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void * accept_request(void *);
void bad_request(int);
static int is_comment(char *);
static char * del_both_trim(char *);
void get_html(int);
void cannot_execute(int);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void post_confirm(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

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

void get_html(int client)
{
    FILE *fp;
    char buf[102400];
    char *delim = "=";
    char *item, *key, *value;
    char ip[16];
    char port[6];
    char baudrate[10];

    int rlen = 1;
    char discard[1024];

    discard[0] = '\0';
    while ((rlen > 0) && strcmp("\n", buf))  // read & discard headers
        rlen = get_line(client, buf, sizeof(buf));

    headers(client, NULL);

    if ((fp = fopen("./.config", "r")) == NULL) {
        if (errno == ENOENT) {
            puts("...httpd: no ./.config, use default parameters");
            return;
        } else
            err_exit("... httpd: open ./.config error");
    }

    //make sure once .config is exist, it is never a blank file
    while ((item = fgets(buf, sizeof(buf), fp)) != NULL) {
        if (is_comment(item))
            continue;

        key = del_both_trim(strtok(buf, delim));
        value = del_both_trim(strtok(NULL, delim));

        if (!value)
            puts_exit("... httpd: config file syntax error");
        if (!strncmp(key, "DPC_IP", 6))
            strcpy(ip, value);
        else if (!strncmp(key, "DPC_PORT", 8))
            strcpy(port, value);
        else if (!strncmp(key, "BAUDRATE", 8))
            strcpy(baudrate, value);
    }
    strcpy(buf,"<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" <meta name=\"viewport\"  content=\"width=device-width,initial-scale=1\">\r\n>");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"<title>TDT configeration</title></head><body><div id=\"wrapper\"></div><h1>TDT 配置页面</h1><form action=\"cgi\" name=\"cgi\"  method=\"post\" >\"\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"<div class=\"col-2\"><label>IP 地址<input placeholder=\"");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,ip);
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"\"id=\"ip\" name=\"ip\" tabindex=\"1\"></label></div>\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"<div class=\"col-2\"><label>端口号<input placeholder=\"");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,port);
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"\"id=\"port\" name=\"port\" tabindex=\"2\"></label></div>\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"<div class=\"col-3\"><label>波特率<input placeholder=\"");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,baudrate);
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"\"id=\"baudrate\" name=\"baudrate\" tabindex=\"3\"></label></div>\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf,"<div class=\"col-3\"><label>WiFi 网络名<input placeholder=\"write here\" id=\"WiFi SSID\" name=\"WiFi SSID\" tabindex=\"4\"></label></div>\r\n");
    send(client, buf, strlen(buf), 0);
	strcpy(buf,"<div class=\"col-3\" ><label>WiFi 密码<input placeholder=\"write here\" id=\"WiFi password\" name=\"WiFi password\" tabindex=\"5\"></label></div>\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf,"<div class=\"col-submit\"><button class=\"submitbtn\" type=\"submit\">提交信息</button></div><div class=\"col-submit\"><button class=\"resetbtn\" type=\"reset\">重置信息</button></div>");
	send(client, buf, strlen(buf), 0);
	strcpy(buf,"<style>@import url(http://fonts.googleapis.com/css?family=Laila:400,700);html, body, div, span, applet, object, iframe, h1, h2, h3, h4, h5, h6, p, blockquote, pre, a, abbr, acronym, address, big, cite, code, del, dfn, em, img, ins, kbd, q, s, samp, small, strike, strong, sub, sup, tt, var, b, u, i, center, dl, dt, dd, ol, ul, li, fieldset, form, label, legend, table, caption, tbody, tfoot, thead, tr, th, td, aside, canvas, details, figure, output, section,   {margin: 0;padding: 0;border: 0;font-size: 100%;font: inherit;vertical-align: baseline;outline: none;-webkit-font-smoothing: antialiased;-webkit-text-size-adjust: 100%;-ms-text-size-adjust: 100%;-webkit-box-sizing: border-box;-moz-box-sizing: border-box;box-sizing: border-box;}");    
    send(client,buf,strlen(buf),0);
	strcpy(buf,"html{ overflow-y: scroll; }body {font-family: Arial, Tahoma, sans-serif;background: #e2eef4;font-size: 62.5%;line-height: 1;padding-top: 20px;}br { display: block; line-height: 1.6em; } input, textarea { -webkit-font-smoothing: antialiased;outline: none; }strong, b { font-weight: bold; }em, i { font-style: italic; }h1 {display: block;font-size: 3.1em;line-height: 1.45em;font-family: 'Laila', sans-serif;text-align: center;font-weight: bold;color: #555;text-shadow: 1px 1px 0 #fff;}");
    send(client,buf,strlen(buf),0);
	strcpy(buf,"form {display: block;margin: 30px;overflow: hidden;background: #fff;border: 1px solid #e4e4e4;border-radius: 5px;font-size: 0;}form > div > label {display: block;padding: 20px 20px 10px;vertical-align: top;font-size: 13px;font-weight: bold;text-transform: uppercase;color: #939393;cursor: pointer;}.col-2, .col-3 { border-bottom: 1px solid #e4e4e4;}label > input {display: inline-block;position: relative;width: 100%;height: 27px;line-height: 27px;margin: 5px -5px 0;padding: 7px 5px 3px;border: none;outline: none;color: #555;font-family: 'Helvetica Neue', Arial, sans-serif;font-weight: bold;font-size: 14px;opacity: .6;transition: all linear .3s;}.col-submit {text-align: center;padding: 5px;}label > input:focus, label > select:focus {opacity: 1;}");
    send(client,buf,strlen(buf),0);
	strcpy(buf,"button {width: 100%;height: 35px;max-width: 150px;border: none;border-radius: 4px;margin: 0 0 15px 0;font-size: 14px;color: #fff;font-weight: bold;text-shadow: 1px 1px 0 rgba(0,0,0,0.3);overflow: hidden;outline: none;}button.submitbtn {background-image: -moz-linear-gradient(#97c16b, #8ab959); background-image: -webkit-linear-gradient(#97c16b, #8ab959);background-image: linear-gradient(#97c16b, #8ab959);border-bottom: 1px solid #648c3a;cursor: pointer;color: #fff;}button.submitbtn:hover {background-image: -moz-linear-gradient(#8ab959, #7eaf4a);background-image: -webkit-linear-gradient(#8ab959, #7eaf4a);background-image: linear-gradient(#8ab959, #7eaf4a);}button.submitbtn:active {height: 34px;border-bottom: 0;margin: 1px 0 0 0;background-image: -moz-linear-gradient(#7eaf4a, #8ab959);background-image: -webkit-linear-gradient(#7eaf4a, #8ab959);background-image: linear-gradient(#7eaf4a, #8ab959);-moz-box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);-webkit-box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);}");
    send(client,buf,strlen(buf),0);
	strcpy(buf,"button.resetbtn {background-image: -moz-linear-gradient(#97c16b, #8ab959);background-image: -webkit-linear-gradient(#97c16b, #8ab959);background-image: linear-gradient(#97c16b, #8ab959);border-bottom: 1px solid #648c3a;cursor: pointer;color: #fff;}button.resetbtn:hover {background-image: -moz-linear-gradient(#8ab959, #7eaf4a);background-image: -webkit-linear-gradient(#8ab959, #7eaf4a);background-image: linear-gradient(#8ab959, #7eaf4a);}button.resetbtn:active {height: 34px;border-bottom: 0;margin: 1px 0 0 0;background-image: -moz-linear-gradient(#7eaf4a, #8ab959);background-image: -webkit-linear-gradient(#7eaf4a, #8ab959);background-image: linear-gradient(#7eaf4a, #8ab959);-moz-box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);-webkit-box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);box-shadow: inset 0 1px 3px 1px rgba(0, 0, 0, 0.3);}");
    send(client,buf,strlen(buf),0);
	strcpy(buf,"@media(min-width: 850px){form > div { display: inline-block; }.col-submit { display: block; }.col-2, .col-3{ box-shadow: 1px 1px #e4e4e4; border: none; }.col-2 { width: 50% }.col-3 { width: 33.3333333333% }.col-submit button { width: 30%; margin: 0 auto; }}</style></form></div></body></html>");
    send(client,buf,strlen(buf),0);
	
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void * accept_request(void *arg)
{
    struct stat st;
    char buf[1024];
    char method[255];
    char url[255];
    char path[512];
    char *query_str = NULL;
    size_t i = 0;
    size_t j = 0;
    int  cgi = 0;
    int  rlen;
    int  client = *(int *)arg;

    rlen = get_line(client, buf, sizeof(buf));

    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        unimplemented(client);
        close(client);
        return((void *)1);
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;

    i = 0;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0) {
        query_str = url;
        while ((*query_str != '?') && (*query_str != '\0'))
            query_str++;
        if (*query_str == '?') {
            cgi = 1;
            *query_str = '\0';
            query_str++;
        }
    }

    sprintf(path, "res%s", url);
    if (path[strlen(path) - 1] == '/')
        get_html(client);
    else if (stat(path, &st) == -1) {
        // read & discard headers
        while ((rlen > 0) && strcmp("\n", buf))
            rlen = get_line(client, buf, sizeof(buf));
        not_found(client);
    } else {
        if ((st.st_mode & S_IXUSR) ||
              (st.st_mode & S_IXGRP) ||
              (st.st_mode & S_IXOTH)) {
            execute_cgi(client, path, method, query_str);
        } else {
            while ((rlen > 0) && strcmp("\n", buf))
            rlen = get_line(client, buf, sizeof(buf));
            not_found(client);
        }
    }

    close(client);
    return((void *)0);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
                 const char *method, const char *query_str)
{
    int   i;
    int   rlen = 1;
    int   clen = -1;
    char  c;
    char  ip[19];
    char  port[12];
    char  baud[13];
    char  buf[1024];
    pid_t pid;

    ip[0]   = '\0';
    buf[0]  = '\0';
    port[0] = '\0';
    baud[0] = '\0';

    if (strcasecmp(method, "GET") == 0)
        while ((rlen > 0) && strcmp("\n", buf))    //read & discard headers
            rlen = get_line(client, buf, sizeof(buf));
    else {    // POST
        rlen = get_line(client, buf, sizeof(buf));
        while ((rlen > 0) && strcmp("\n", buf)) {
            if (clen == -1) {
                buf[15] = '\0';
                if (strcasecmp(buf, "Content-Length:") == 0)
                    clen = atoi(&(buf[16]));
            }
            rlen = get_line(client, buf, sizeof(buf));
        }
        if (clen == -1) {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if ((pid = fork()) < 0)
        err_exit("fork");
    if (pid == 0) {    //child cgi
        char query_env[255];

        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "query_str=%s", query_str);
            putenv(query_env);
        } else {   //POST
            for (i = 0; i < clen; i++) {
                recv(client, &c, 1, 0);
                if (c != '&')
                    strncat(ip, &c, 1);
                else {
                    putenv(ip);
                    i++;
                    break;
                }
            }
            for (; i < clen; i++) {
                recv(client, &c, 1, 0);
                if (c != '&')
                    strncat(port, &c, 1);
                else {
                    putenv(port);
                    i++;
                    break;
                }
            }
            for (; i < clen; i++) {
                recv(client, &c, 1, 0);
                strncat(baud, &c, 1);
            }
            putenv(baud);
        }
        execl(path, path, NULL);
        exit(0);
    } else {    //parent
        wait(NULL);
        headers(client, NULL);
        post_confirm(client);
    }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    char c = '\0';
    int  i = 0;
    int ret;

    while ((i < size - 1) && (c != '\n')) {
        if (recv(sock, &c, 1, 0) > 0) {
            if (c == '\r') {
                ret = recv(sock, &c, 1, MSG_PEEK);
                if ((ret > 0) && (c == '\n'))    //bypass the "\r\n" ?
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }

    buf[i] = '\0';

    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  // could use filename to determine file type

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void post_confirm(int client)
{
    char buf[1024];

    sprintf(buf, "<HTML><HEAD><TITLE>TDT Configuration\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><h1>Succeed</h1>");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<a href=\"http://192.168.1.1/index.html\">Back</a>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int rlen = 1;
    char buf[1024];

    buf[0] = '\0';
    while ((rlen > 0) && strcmp("\n", buf))  // read & discard headers
        rlen = get_line(client, buf, sizeof(buf));

    if ((resource = fopen(filename, "r")) == NULL)
        not_found(client);
    else {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
    int sk;
    struct sockaddr_in addr;

    if ((sk = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        err_exit("socket");

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(*port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        err_exit("bind");
    // if dynamically allocating a port
    if (*port == 0) {
        socklen_t addrlen = sizeof(struct sockaddr_in);
        if (getsockname(sk, (struct sockaddr *)&addr, &addrlen) == -1)
            err_exit("getsockname");
        *port = ntohs(addr.sin_port);
    }
    if (listen(sk, 5) < 0)
        err_exit("listen");

    return(sk);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)
{
    int server_sock;
    u_short port = 80;
    int client = -1;
    pthread_t tid;

    server_sock = startup(&port);

    printf("httpd running on port %d\n", port);

    for (;;) {
        if ((client = accept(server_sock, NULL, NULL)) == -1)
            err_exit("accept");

        printf("accept a request\n");

        if (pthread_create(&tid , NULL, accept_request, (void *)&client) != 0)
            err_exit("pthread_create");
    }

    close(server_sock);

    exit(0);
}

