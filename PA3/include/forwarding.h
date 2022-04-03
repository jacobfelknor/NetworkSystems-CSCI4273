#include "../include/str_utils.h"
#include "../include/utils.h"

void http_forward(int connfd, char *responseBuffer, long *responseSize, char *requestMethod, char *requestPath, char *httpVersion);