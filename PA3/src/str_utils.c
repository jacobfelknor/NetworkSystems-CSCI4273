#include "../include/str_utils.h"

// https://stackoverflow.com/a/15515276
bool startsWith(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

// https://stackoverflow.com/a/20300544
// get a line from a buffer
char *sgets(char *s, int n, char **strp)
{
    if (**strp == '\0')
        return NULL;
    int i;
    for (i = 0; i < n - 1; ++i, ++(*strp))
    {
        s[i] = **strp;
        if (**strp == '\0')
            break;
        if (**strp == '\n')
        {
            s[i + 1] = '\0';
            ++(*strp);
            break;
        }
    }
    if (i == n - 1)
        s[i] = '\0';
    return s;
}

void splitRequestString(char *request, char *requestMethod, char *requestPath, char *httpVersion)
{
    // For these 3 pieces of information, only the first line is required.
    int lineSize = 300;
    char firstLine[lineSize]; // should be plenty long
    sgets(firstLine, lineSize, &request);

    char *parts[] = {requestMethod, requestPath, httpVersion};
    int part = 0;
    int j = 0;
    // split string into parts by space
    for (int i = 0; i <= strlen(firstLine); i++)
    {
        if (part > 2)
        {
            // reached the end of available parts. If its not done, we can't parse this http request
            break;
        }
        // adapted from includehelp.com/c-programs/c-program-to-split-string-by-space-into-words.aspx
        if (firstLine[i] == ' ' || firstLine[i] == '\0')
        {
            parts[part][j] = '\0';
            part++; // for next part
            j = 0;  // for next part, init index to 0
        }
        else
        {
            parts[part][j] = firstLine[i];
            j++;
        }
    }

    // at this point, replace the newline if it exists from the httpVersion
    httpVersion[strcspn(httpVersion, "\n")] = 0;
    httpVersion[strcspn(httpVersion, "\r")] = 0;
}

// adapted from https://stackoverflow.com/a/58065405
char *computeMD5Path(char *str)
{
    int DIGEST_STR_SIZE = 33;
    char *cachePath = "cache/";
    int bufsize = strlen(cachePath) + DIGEST_STR_SIZE;
    char *digestPath = (char *)malloc(bufsize);
    bzero(digestPath, bufsize);
    strcpy(digestPath, cachePath);

    char *digeststr = digestPath + strlen(cachePath);
    unsigned char digest[16];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str, strlen(str));
    MD5_Final(digest, &ctx);

    for (int i = 0, j = 0; i < 16; i++, j += 2)
    {
        sprintf(digeststr + j, "%02x", digest[i]);
    }

    return digestPath;
}