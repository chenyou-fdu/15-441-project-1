#include <iostream>
#include "http_conn.h"


/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "http_conn.h"
#include <string>
#include <iostream>

#include "parse_wrapper.h"

#include <chrono>
#include <unordered_map>

using namespace std;
unordered_map<int, HttpConn*> httpConnMap;
string write_back;

#define ECHO_PORT 9999
#define BUF_SIZE 4096
#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("error no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

bool send_response(int efd, int fd);
void setNonBlock(int fd);
void updateEvents(int efd, int fd, int events, int op);
void handleAccept(int efd, int fd);
void handleRead(int efd, int fd);
int handleTimeout(int efd, int tfd);
void loop_once(int efd, int lfd, int tfd, int waitms);
int create_timerfd(int msec);



int main(int argc, char* argv[])
{
    int epollfd = epoll_create(1);
    exit_if(epollfd < 0, "epoll_create failed");
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    exit_if(listenfd < 0, "socket failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    int port = 9999;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int r = bind(listenfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    exit_if(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));
    r = listen(listenfd, 20);
    exit_if(r, "listen failed %d %s", errno, strerror(errno));
    printf("fd %d listening at %d\n", listenfd, port);
    setNonBlock(listenfd);
    // binding listen fd to epoll fd
    updateEvents(epollfd, listenfd, EPOLLIN, EPOLL_CTL_ADD);
    // binding timer fd
    int tfd = create_timerfd(10);
    updateEvents(epollfd, tfd, EPOLLIN | EPOLLET, EPOLL_CTL_ADD);
    setNonBlock(tfd);
    for (;;) {
        loop_once(epollfd, listenfd, tfd, 10000);
    }
}


bool send_response(int efd, int fd) {
    HttpConn* httpConnPtr = httpConnMap[fd];
    size_t left_to_write = write_back.length() - httpConnPtr->written_len;
    size_t n;
    while ((n = write(fd, write_back.data() + httpConnPtr->written_len, left_to_write)) > 0) {
        httpConnPtr->written_len += n;
        left_to_write -= n;
    }
    // remove fd when no data to write in LT mode
    if(left_to_write == 0) {
        if(httpConnPtr->written_enabled) {
            updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_MOD);
            httpConnPtr->written_enabled = false;
        }
        httpConnMap.erase(fd);
        //close(fd);
        //cout << "fd "  << fd << " this time " << write_back << endl;
        return true;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        if(!httpConnPtr->written_enabled) {
            updateEvents(efd, fd, EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
            httpConnPtr->written_enabled = true;
        }
        return false;
    }
    /*
    if(!httpConnPtr->written_enabled) {
        httpConnPtr->written_enabled = true;
        updateEvents(efd, fd, EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
    }*/
    if (n <= 0) {
        printf("write error for %d: %d %s\n", fd, errno, strerror(errno));
        close(fd);
        httpConnMap.erase(fd);
        // TODO what?
        return true;
    }
    return false;
}

void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");
    int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r < 0, "fcntl failed");
}

void updateEvents(int efd, int fd, int events, int op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    printf("%s fd %d events read %d write %d\n",
           op == EPOLL_CTL_MOD ? "mod" : "add", fd,
           ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int r = epoll_ctl(efd, op, fd, &ev);
    exit_if(r, "epoll_ctl failed");
}

void handleAccept(int efd, int fd) {
    struct sockaddr_in raddr;
    socklen_t raddr_len = sizeof(raddr);
    int cfd = accept(fd, (struct sockaddr *)&raddr, &raddr_len);
    exit_if(cfd < 0, "accept failed");
    /*
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int r = getpeername(cfd, (sockaddr*)&peer, &alen);
    exit_if(r<0, "getpeername failed");
    */
    printf("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));
    setNonBlock(cfd);
    //updateEvents(efd, cfd, EPOLLIN | EPOLLOUT, EPOLL_CTL_ADD);
    updateEvents(efd, cfd, EPOLLIN, EPOLL_CTL_ADD);
}

void handleRead(int efd, int fd) {
    char buf[4096];
    int n = 0;
    HttpConn* httpConnPtr = nullptr;
    if(httpConnMap.find(fd) == httpConnMap.end()) {
        HttpConn httpConn(fd);
        httpConnMap.insert({fd, &httpConn});
        httpConnPtr = &httpConn;
    } else {
        httpConnPtr = httpConnMap.find(fd)->second;
    }
    while((n = read(fd, buf, 4096)) > 0) {
        //printf("read %d bytes with value %s", n, buf);
        httpConnPtr->buf.append(buf, n);
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto value = now_ms.time_since_epoch();
        httpConnPtr->ts = value.count();
        if(httpConnPtr->buf.length() >= 4) {
            size_t found = httpConnPtr->buf.find("\r\n\r\n");
            if(found == string::npos) {
                cout << "can't parsed yet" << endl;
            } else {
                Request* req = parseNew(httpConnPtr->buf.c_str(), httpConnPtr->buf.size());
                if(req != nullptr) {
                    write_back = httpConnPtr->buf;
                } else {
                    write_back = "HTTP/1.1 400 Bad Request\r\n";
                }
                send_response(efd, fd);
                break;
            }
        }
    }
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
    }
    if (n <= 0) {
        printf("error no: %d error msg %s\n", errno, strerror(errno));
        close(fd);
        httpConnMap.erase(fd);
    }
    //exit_if(n < 0, "read error! ");
    //printf("fd %d closed\n", fd);
    //close(fd);
}

int handleTimeout(int efd, int tfd) {
    uint64_t fake_num;
    read(tfd, &fake_num, 8);
    int cnt = 0;
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    long now_ms_long = value.count();
    for(auto it = httpConnMap.begin(); it != httpConnMap.end(); it++) {
        long last_update_ts = it->second->ts;
        if(now_ms_long - last_update_ts > 15000) {
            printf("fd %d time out\n", it->first);
            //httpConnMap.erase(it);
            //close(it->first);
            write_back = "HTTP/1.1 400 Bad Request\r\n";
            send_response(efd, it->first);
        }
    }
    return cnt;
}


void loop_once(int efd, int lfd, int tfd, int waitms) {
    const int kMaxEvents = 20;
    struct epoll_event activeEvs[100];
    int n = epoll_wait(efd, activeEvs, kMaxEvents, waitms);
    printf("epoll_wait return %d\n", n);
    if(n < 0) {
        if(errno != EINTR){
            return;
        }
    }
    for (int i = 0; i < n; i ++) {
        int fd = activeEvs[i].data.fd;
        int events = activeEvs[i].events;
        if(events & EPOLLIN) {
            if(fd == lfd) {
                handleAccept(efd, fd);
            } else if(fd == tfd) {
                printf("handle timeout\n");
                handleTimeout(efd, tfd);
            } else {
                handleRead(efd, fd);
            }
        } else if(events & EPOLLERR) {

        } else if(events & EPOLLOUT) {
            if(fd != lfd) {
                cout << "can write" << endl;
            }
            send_response(efd, fd);
        }
        /*
        if (events & (EPOLLIN | EPOLLERR)) {
            if (fd == lfd) {
                //handleAccept(efd, fd);
            } else {
                //handleRead(efd, fd);
            }
        } else if (events & EPOLLOUT) {
            //handleWrite(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
         */
    }
}

int create_timerfd(int sec) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        printf("timerfd_create() failed: errno=%d\n", errno);
        return tfd;
    }

    struct itimerspec ts;
    bzero(&ts, sizeof(ts));
    //int msec = 500; // timer fires after 500 msec
    /*
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    //ts.it_value.tv_sec = msec / 1000;
    ts.it_value.tv_sec = 3;
    //ts.it_value.tv_nsec = (msec % 1000) * 1000000;
     */
    struct timespec tspec;
    tspec.tv_sec = sec;
    tspec.tv_nsec = 0;
    ts.it_value = tspec;
    ts.it_interval = tspec;

    if (timerfd_settime(tfd, 0, &ts, nullptr) < 0) {
        printf("timerfd_settime() failed: errno=%d\n", errno);
        close(tfd);
        return -1;
    }

    return tfd;
}
