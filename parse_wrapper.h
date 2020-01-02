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

RequestNew* parseNew(const char *buffer, int size) {
    buffer = "GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n";
    char* newBuffer = const_cast<char *>(buffer);
    size = strlen(buffer);
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    /*
    int minor_version;
    //buffer[31] = '\r';
    const char *method, *path;
    phr_header* headers = new phr_header[100];
    int pret = phr_parse_request(buffer, size, &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &num_headers, prevbuflen);
    if(pret < 0) {
        return nullptr;
    } else {
        return new Request(minor_version, method, method_len, path, path_len, headers, num_headers);
    }
    struct http_parser parser;
    http_parser_settings settings;
    int i;
    int err;
    size_t parsed;

    http_parser_settings_init(&settings);
    http_parser_init(&parser, HTTP_REQUEST);
    parsed = http_parser_execute(&parser, &settings, buffer, size);
    return new RequestNew();
    */
    Request* shit = parse(newBuffer, size);
    return NULL;
}



#endif //HTTP_SERVER_PARSE_WRAPPER_H
