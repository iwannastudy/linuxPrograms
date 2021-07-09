#include "network.h"

extern PFDITEM gFDITEM[MAX_FD];
int process_connection(int worker_epollfd, char *buffer, int &received,
                  TIMER *timerlink);

int readfun_getConnections(int worker_epollfd, int orderfd, TIMER *timerlink) 
{
    assert(worker_epollfd != 0);
    assert(orderfd != 0);
    char buffer[10 * 1024] = {0}; //2.5k fd
    int received = 0;
    int rest = 0;
    while (true)
    {
        rest = sizeof(buffer) - received;

        int count = recv(orderfd, buffer + received, rest, 0);
        if (count > 0 && ((size_t)(received + count) >= sizeof(int)))  // received at least 1 fd
        {
            received += count;
            process_connection(worker_epollfd, buffer, received, timerlink);
        }
        else if (count == -1 && errno == EAGAIN)
        {
            // have no data to recv for a non-block I/O
            break;
        } 
        else
        {
            // should not get here
            assert(false);
        }
    }
    return 1;
}

int process_connection(int worker_epollfd, char *buffer, int &received, TIMER *timerlink) 
{
    unsigned int processed = 0;
    struct epoll_event ev = {0};

    while (true)
    {
        TimeValue timeStamp = getNowTimeStamp();
        if ( (int)(processed + sizeof(int)) <= received )
        {
            // process 1 fd at one time
            int *pFd = (int *)(buffer + processed);
            processed += sizeof(int);
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = *pFd;
            set_noblock(*pFd);

            gFDITEM[*pFd]->m_readfun = client_readfun;
            gFDITEM[*pFd]->m_writefun = client_writefun;
            gFDITEM[*pFd]->m_closefun = client_closefun;
            gFDITEM[*pFd]->m_timeoutfun = client_timeoutfun;
            if (epoll_ctl(worker_epollfd, EPOLL_CTL_ADD, *pFd, &ev) == -1)
            {
                perror("epoll_ctl: listen_sock");
                exit(-1);
            }
            else
            {
                timerlink->add_timer(gFDITEM[*pFd], timeStamp + 10 * 1000);  // 10s timer
            }
        }
        else
        {
            break;
        }
    }

    if (int(processed) == received)
    {
        received = 0;
    } 
    else    // 3 >= (received - processed) >= 1
    {
        memmove(buffer, buffer + processed, received - processed);
        received -= processed;
    }
    return 0;
}

void *worker_thread(void *arg) 
{

    struct WorkerThreadArgs *args = (struct WorkerThreadArgs *)arg;

    struct epoll_event *evlist;

    int worker_epollfd;
    assert(args);
    struct epoll_event ev = {0};

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = args->orderfd;
    set_noblock(args->orderfd);

    evlist = (struct epoll_event *)malloc(MAXEPOLLEVENT *
                                            sizeof(struct epoll_event));
    worker_epollfd = epoll_create(MAXEPOLLEVENT);
    gFDITEM[args->orderfd]->m_readfun = readfun_getConnections;
    if (epoll_ctl(worker_epollfd, EPOLL_CTL_ADD, args->orderfd, &ev) == -1) 
    {
        perror("epoll_ctl: listen_sock");
        exit(-1);
    } 

    TIMER global_timer;
    TimeValue outtime = 1000;
    while (true)
    {
        process_event(worker_epollfd, evlist, outtime, &global_timer);
        if (global_timer.get_arg_time_size() > 0)
        {
            outtime = global_timer.get_mintimer();
        } 
        else
        {
            outtime = 1000;
        }
    }
}

int client_readfun(int epoll, int fd, TIMER* timer) 
{
    char buff[1024];

    while (true) 
    {
        int len = read(fd, buff, sizeof(buff) - 1);
        if (len == -1) 
        {
            buff[0] = 0;
            break;

        }
        else if (len == 0)
        {
            buff[len] = 0;
            gFDITEM[fd]->m_activeclose = true;
            return -1;
        }
        if (len > 0)
        {
            Log << "'" << buff << "'" << std::endl;
        }
    }
    return 1;
}

char senddata[2048];
const int presend = 500;
int client_writefun(int epoll, int fd, TIMER* timer)
{

    int len = presend - gFDITEM[fd]->m_sended;
    len = write(fd, &(senddata[0]) + gFDITEM[fd]->m_sended, len);
    if (len == 0)
    {
        gFDITEM[fd]->m_activeclose = true;
    }
    else if (len > 0)
    {
        gFDITEM[fd]->m_sended += len;
        if (gFDITEM[fd]->m_sended == presend)
        {
            gFDITEM[fd]->m_sended = 0;
        }
    }
    return 1;
}

int client_closefun(int epoll, int fd, TIMER* timer)
{
    timer->remove_timer(gFDITEM[fd]);

    gFDITEM[fd]->init();

    ::close(fd);
    return 1;
}
int client_timeoutfun(int epoll, int fd, TIMER* timers, TimeValue tnow)
{

    int len = presend - gFDITEM[fd]->m_sended;
    len = write(fd, &(senddata[0]) + gFDITEM[fd]->m_sended, len);
    if (len == 0)
    {
        gFDITEM[fd]->m_activeclose = true;
    }
    else if (len > 0)
    {
        gFDITEM[fd]->m_sended += len;
        if (gFDITEM[fd]->m_sended == presend)
        {
            gFDITEM[fd]->m_sended = 0;
        }
    }
    timers->add_timer(gFDITEM[fd], tnow + 10 * 1000);
    return 0;
}
