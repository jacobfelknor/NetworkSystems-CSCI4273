#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

char *sgets(char *s, int n, char **strp);

void splitRequestString(char *request, char *requestType, char *requestPath, char *httpVersion);