#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <openssl/md5.h>

char *sgets(char *s, int n, char **strp);

void splitRequestString(char *request, char *requestMethod, char *requestPath, char *httpVersion);

bool startsWith(const char *a, const char *b);

char *computeMD5Path(char *str);