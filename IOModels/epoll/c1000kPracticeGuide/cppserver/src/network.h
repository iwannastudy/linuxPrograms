#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include "timers.h"

#include <string>
#include <iostream>
#include <sched.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include "socket_util.h"

const int K = 1024;

// 220 万连接数
const int MAX_FD = 220 * 10 * K;

// 每次处理event 数目
const int MAXEPOLLEVENT = 10000;

const int MAXACCEPTSIZE = 1024;

#define MAX_CPU 128

typedef int (*readfun)(int epoll, int fd, TIMER *);
typedef int (*writefun)(int epoll, int fd, TIMER *);
typedef int (*closefun)(int epoll, int fd, TIMER *);
typedef int (*timeoutfun)(int epoll, int fd, TIMER *, TimeValue tnow);

int accept_readfun(int epoll, int listenfd, TIMER *);
int accept_write(int epoll, int listenfd, TIMER *);

void *dispatch_conn(void *);

int fdsend_writefun(int epollfd, int listenfd, TIMER *timerlink);

int client_readfun(int epoll, int fd, TIMER *timer);
int client_writefun(int epoll, int fd, TIMER *timer);
int client_closefun(int epoll, int fd, TIMER *timer);
int client_timeoutfun(int epoll, int fd, TIMER *, TimeValue tnow);

int readfun_getConnections(int epoll, int listenfd, TIMER *);

struct WorkerThreadArgs {
    int orderfd;  // listen 线程会用这个发送fd过来
    int cpuid;
};

struct Accepted_FD 
{
    int     acceptedFdLen;
    char    acceptedFdBuff[MAXACCEPTSIZE * 4];
    struct  Accepted_FD *next;
};

class FDITEM
{
public:
    FDITEM() { init(); }

    readfun m_readfun;
    writefun m_writefun;
    closefun m_closefun;
    timeoutfun m_timeoutfun;
    int fd;
    int m_sended;
    TimeValue m_lastread;
    TimeValue m_timeout;  // timeout的时间
    bool m_activeclose;

    void init()  // default to inline, but will be optimized by complier
    {
        m_readfun = NULL;
        m_writefun = NULL;
        m_closefun = NULL;
        m_timeoutfun = NULL;
        m_lastread = getNowTimeStamp();
        m_sended = 0;
        m_activeclose = false;
    }
};
typedef FDITEM* PFDITEM;

struct SendFdBuff 
{
    int needSendLen;
    char buff[4 * 30000];
};

#define Log std::cerr

bool connect(int &fd, std::string host, int port);

const int listenport = 8888;

bool addtimer(TIMER *, int fd, TimeValue);

bool checkontimer(TIMER *, int *fd, TimeValue *);

bool stoptimer(TIMER *, int fd, TimeValue);

TimeValue getnexttimer(TIMER *);

void process_event(int epollfd, struct epoll_event *m_events,
                   TimeValue timeout, TIMER *timers);

void *worker_thread(void *arg);

void *main_thread(void *arg);

void *distpatch_thread(void *arg);

#endif
