/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
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

#define ISspace(x) isspace((int)(x))

#define err_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void * accept_request(void *);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void post_confirm(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

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
        strcat(path, "index.html");
    if (stat(path, &st) == -1) {
        /* read & discard headers */
        while ((rlen > 0) && strcmp("\n", buf))
            rlen = get_line(client, buf, sizeof(buf));
        not_found(client);
    } else {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        else if ((st.st_mode & S_IXUSR) ||
              (st.st_mode & S_IXGRP) ||
              (st.st_mode & S_IXOTH))
            cgi = 1;
        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_str);
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
    char  port[11];
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
    (void)filename;  /* could use filename to determine file type */

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
    while ((rlen > 0) && strcmp("\n", buf))  /* read & discard headers */
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
    /* if dynamically allocating a port */
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
