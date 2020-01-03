#ifndef HTTP_SERVER_PARSE_WRAPPER_H
#define HTTP_SERVER_PARSE_WRAPPER_H

#include <cstring>
extern "C" {
#include "parser_lib/parse.h"
};
struct RequestNew
{
    int http_version;
    const char* method;
    const char* path;
    size_t path_len;
    int method_len;
    size_t header_count;
    RequestNew() {

    }
    RequestNew(int v, const char* m, size_t m_len, const char* p, size_t p_len, size_t h_c) : http_version(v),
                                                                                                            method(m), method_len(m_len), path(p), path_len(p_len), header_count(h_c){}
};

Request* parseNew(const char *buffer, int size) {
    //buffer = "GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n";
    char* newBuffer = const_cast<char *>(buffer);
    //size = strlen(buffer);
    //size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    return parse(newBuffer, size);
}



#endif //HTTP_SERVER_PARSE_WRAPPER_H
