#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

char *sgets(char *s, int n, char **strp);

void splitRequestString(char *request, char *requestMethod, char *requestPath, char *httpVersion);

bool startsWith(const char *a, const char *b);

char *strConcat(const char *s1, const char *s2);