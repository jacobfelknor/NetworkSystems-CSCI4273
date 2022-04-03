#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../include/forwarding.h"

// adapted from https://stackoverflow.com/a/726214
void split_url(char *requestPath, char *host, char *page)
{

    sscanf(requestPath, "http://\%[^/]%99[^\n]", host, page);
    if (strlen(page) == 0)
    {
        page[0] = '/';
    }
    // printf("host = \"%s\"\n", host);
    // printf("page = \"%s\"\n", page);
    // strcpy(requestPath, page)
}

int get_socket(char *hostname)
{
    int sockfd;                    /* socket */
    struct sockaddr_in serveraddr; /* server's addr */
    struct hostent *host;          /* host info */
    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    // get hostname
    /* gethostbyname: get the server's DNS entry */
    host = gethostbyname(hostname);
    if (host == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)host->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, host->h_length);
    // TODO: make the port an argument
    serveraddr.sin_port = htons(8000);

    // connect
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)) == -1)
    {
        error("connection refused");
    }

    return sockfd;
}

void http_forward(int connfd, char *request)
{

    // parse first line of request for our 3 main substrings (with conservatively long buffers...)
    char requestMethod[20]; // e.g. GET, POST, etc
    char hostname[100];     // e.g. www.nginx.org
    char requestPath[260];  // e.g. /some/dir/page.html
    char page[200];
    bzero(page, 200);
    char httpVersion[20]; // e.g. HTTP/1.1
    long responseSize;
    char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);

    // const char *root = "./www"; // our document root relative path
    // strcpy(requestPath, root);  // prepend our root to this string.
    splitRequestString(request, requestMethod, requestPath, httpVersion);
    split_url(requestPath, hostname, page);

    if (strcmp(requestMethod, "GET") != 0)
    {
        // i'm just ignoring other requests for now because they're anoying
        return;
    }
    // printf("%s\n", request);
    // printf("%s\n%s\n", hostname, page);

    int sockfd = get_socket(hostname);
    // setup complete. Send request and capture the response

    char *myrequest = (char *)malloc(1000);
    snprintf(myrequest, 1000,
             "GET %s %s\r\n\r\n", page, httpVersion);
    printf("%s\n", myrequest);
    write(sockfd, myrequest, strlen(myrequest));

    // read back response from server
    int bytesRead = 0;
    int chunk = 1;
    while (chunk > 0)
    {
        chunk = read(sockfd, responseBuffer + bytesRead, RESPONSE_BUFFER_SIZE - bytesRead);
        bytesRead += chunk;
    };

    // read(sockfd, responseBuffer, 1000);

    // printf("%s\n", responseBuffer);

    sendResponse(connfd, responseBuffer, bytesRead);

    // printf("I would fetch and reply from here...\n");
}