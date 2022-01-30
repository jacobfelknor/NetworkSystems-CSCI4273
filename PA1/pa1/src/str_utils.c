#include "../include/str_utils.h"

// https://stackoverflow.com/a/15515276
bool startsWith(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

// https://stackoverflow.com/a/1488419
char *strstrip(char *s)
{
    size_t size;
    char *end;

    size = strlen(s);

    if (!size)
        return s;

    end = s + size - 1;
    while (end >= s && isspace(*end))
        end--;
    *(end + 1) = '\0';

    while (*s && isspace(*s))
        s++;

    return s;
}

// https://stackoverflow.com/a/4761840
size_t chopN(char *str, size_t n)
{
    // assert(n != 0 && str != 0);
    // char temp[n+1];
    // memccpy(temp, str, n);
    size_t len = strlen(str);
    if (n > len)
        n = len;
    memmove(str, str + n, len - n + 1);
    // bzero(str + n - 1, BUFSIZE - n);
    return (len - n);
}