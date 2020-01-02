#ifndef HTTP_SERVER_HTTP_CONN_H
#define HTTP_SERVER_HTTP_CONN_H

#include <string>
#include <vector>
using namespace std;
class HttpConn {
public:
    HttpConn(int f) : fd(f), buf(""), ts(0), written_len(0), written_enabled(false) {}
    int fd;
    size_t written_len;
    string buf;
    long ts;
    bool written_enabled;
    /*
    ~HttpConn() {
        delete[] buf;
    }*/
};

#endif //HTTP_SERVER_HTTP_CONN_H
