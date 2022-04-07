#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../include/forwarding.h"

#define MAX_REQUEST_LENGTH 1000

// adapted from https://stackoverflow.com/a/726214
void split_url(char *requestPath, char *host, int *port, char *page)
{
    // first, split the host:port and /page
    sscanf(requestPath, "http://%[^/]%99[^\n]", host, page);
    // now, split the host into the true host:port
    *port = -1;
    sscanf(host, "%*[^:]:%d", port);
    if (*port == -1)
    {
        // port not specified. Use 80
        *port = 80;
    }
    else
    {
        // truncate the host string at the :
        host[strcspn(host, ":")] = 0;
    }
    // if we don't have a path given, just use /
    if (strlen(page) == 0)
    {
        page[0] = '/';
    }
}

bool hostBlocked(char *host)
{
    // open blocklist, search file for host, then return if I find it
    FILE *blocklist = fopen("blocklist", "r");
    char *found = NULL;
    if (blocklist == NULL)
    {
        // the file either DNE or cannot be read. Won't block anything in this case
        return false;
    }
    else
    {
        // put file into memory. Makes finding the hosts easier.
        long fileSize = getFileSize(blocklist);
        if (fileSize > 0)
        {
            char *fileBuffer = (char *)malloc(fileSize);
            putFileInBuffer(fileBuffer, fileSize, blocklist);
            // use strstr to attempt to find the substring
            found = strstr(fileBuffer, host);
            bool block = (found != NULL);
            free(fileBuffer);
            fclose(blocklist);
            return block;
        }
        fclose(blocklist);
    }
    return false;
}

// adapted from https://stackoverflow.com/q/44763040
char *host2ip(struct hostent *host)
{
    // translate hostent to string ip
    // xxx.xxx.xxx.xxx -->
    char *hostname = (char *)malloc(strlen("xxx.xxx.xxx.xxx"));
    bzero(hostname, strlen("xxx.xxx.xxx.xxx"));
    int j = 0;
    for (int i = 0; i < host->h_length; i++)
    {
        j += sprintf(hostname + j, "%d", (unsigned char)host->h_addr_list[0][i]);
        if (i != host->h_length - 1)
        {
            hostname[j] = '.';
            j += 1;
        }
    }

    return hostname;
}

int get_socket(char *hostname, int port)
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
        // error("no such host");
        // on dns lookup failure, return -1
        return -1;
    }
    else
    {
        char *ip = host2ip(host);
        printf("%s (%s)\n", hostname, ip);
        if (hostBlocked(ip) || hostBlocked(hostname))
        {
            // on blocked host, return -2
            free(ip);
            return -2;
        }
        free(ip);
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)host->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, host->h_length);
    serveraddr.sin_port = htons(port);

    // connect
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)) == -1)
    {
        error("Error on connect");
    }

    return sockfd;
}

void http_forward(int connfd, char *responseBuffer, long *responseSize, char *requestMethod, char *requestPath, char *httpVersion)
{
    char hostname[100]; // e.g. www.nginx.org
    int port;           // e.g. 80
    char page[200];
    bzero(page, 200);

    // extract our host and port
    split_url(requestPath, hostname, &port, page);

    // get a socket for use to use to communicate to the webserver
    int sockfd = get_socket(hostname, port);
    if (sockfd == -1)
    {
        // get_socket failed in gethostbyname. Return 404 Not Found per lab instructions
        *responseSize = buildResponse(responseBuffer, httpVersion, "404 Not Found", "text/html", 0);
        return;
    }
    else if (sockfd == -2)
    {
        // get_socket detected a blocked ip address. Return 403 Forbidden
        *responseSize = buildResponse(responseBuffer, httpVersion, "403 Forbidden", "text/html", 0);
        return;
    }

    // setup complete. Send request and capture the response
    char *myrequest = (char *)malloc(MAX_REQUEST_LENGTH);
    snprintf(myrequest, MAX_REQUEST_LENGTH,
             "GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", page, httpVersion, hostname);
    printf("%s\n", myrequest);
    write(sockfd, myrequest, strlen(myrequest));

    // read back response from server
    *responseSize = 0;
    int chunk = 1;

    // 2 strategies here:
    //      - Specify the Connection: close header so that read() doesn't continue to block for
    //        x seconds until timeout specified by server is met

    //      - Parse the Content-Length header and read until we've gotten all the bytes we're expecting
    //           - unforntuately, not everyone is nice and includes this header. So, I wouldn't get all the
    //             information because I wouldn't know when to stop and read() could block if the server
    //             kept the connection alive
    //           - httpforever.com didn't include the header :( I was using them to test
    //           - neverssl.com DOES include header :)

    // int totalContentLength = 0;
    // while (bytesRead < totalContentLength)
    while (chunk > 0)
    {
        chunk = read(sockfd, responseBuffer + *responseSize, RESPONSE_BUFFER_SIZE - *responseSize);
        // Content-Length parsing below....
        // if (totalContentLength == 0)
        // {
        //     // find Content-Length. Use this to decide if we've finished reading.
        //     char *position = strstr(responseBuffer, "Content-Length");
        //     char *endOfHeaders = strstr(responseBuffer, "\r\n\r\n") + strlen("\r\n\r\n");
        //     int lengthOfHeaders = endOfHeaders - responseBuffer;
        //     if (position != NULL)
        //     {

        //         sscanf(position, "Content-Length: %d", &totalContentLength);
        //     }
        //     else
        //     {
        //         // Content-Length is unspecified.
        //     }
        //     totalContentLength += lengthOfHeaders;
        // }
        *responseSize += chunk;
    };

    free(myrequest);
}