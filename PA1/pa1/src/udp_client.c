/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#include "../include/str_utils.h"
#include "../include/file_transfer.h"
#include "../include/utils.h"
#include "../include/constants.h"

int main(int argc, char **argv)
{
  int sockfd, portno, n;
  int serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];

  /* check command line arguments */
  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
    exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
        (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  /* get a message from the user */
  while (1)
  {

    bzero(buf, BUFSIZE);
    printf("cmd: ");
    fgets(buf, BUFSIZE, stdin);
    // Check command that was sent. Respond accordingly
    char *stripped;
    // strip string of leading/trailing spaces
    stripped = strstrip(buf);
    if (startsWith(stripped, "put"))
    {
      // remove delete keyword to obtain filename
      size_t len;
      len = chopN(stripped, strlen("put") + 1);
      // strip string of leading/trailing spaces
      stripped = strstrip(stripped);
      if (strlen(stripped) == 0)
      {
        // case where user sends "delete" but no filenames
        printf("Usage: put must be followed by a filename\n");
        continue;
      }
      else
      {
        // printf("Filename to put %s\n", stripped);
        FILE *fp;
        fp = fopen(stripped, "r");
        if (fp == NULL)
        {
          printf("Error opening specified file. It may not exist on this host\n");
          continue;
        }
        else
        {
          // file open successful. Send file
          send_file(fp, buf, sockfd, serveraddr);
          continue;
        }
      }
    }
    else
    {
      /* send the message to the server */
      serverlen = sizeof(serveraddr);
      n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      if (n < 0)
        error("ERROR in sendto");
    }

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* Check for goodbye msg */
    if (strcmp(buf, "Goodbye") == 0)
    {
      printf("\n%s\n", buf);
      exit(0);
    }
    else if (startsWith(buf, "START"))
    {
      // the server is trying to send us a file!
      write_file(sockfd, buf, serveraddr);
      printf("Wrote file to cwd\n");
    }
    else
    {
      printf("\n%s\n", buf);
    }
  }
  return 0;
}
