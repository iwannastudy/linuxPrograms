#include "network.h"

extern PFDITEM gFDITEM[MAX_FD];

void process_one_event(int epollfd, struct epoll_event *evlist,
                       TIMER *timer) 
{
    int fd = evlist->data.fd;

    if ((!gFDITEM[fd]->m_activeclose) && (evlist->events & EPOLLIN))
    {
        if (gFDITEM[fd]->m_readfun)
        {
            if (0 > gFDITEM[fd]->m_readfun(epollfd, fd, timer))
            {
                gFDITEM[fd]->m_activeclose = true;
            }
        }
    }

    if ((!gFDITEM[fd]->m_activeclose) && (evlist->events & EPOLLOUT)) 
    {
        if (gFDITEM[fd]->m_writefun)
        {
            if (0 > gFDITEM[fd]->m_writefun(epollfd, fd, timer))
            {
                gFDITEM[fd]->m_activeclose = true;
            }
        }
    }

    if ((!gFDITEM[fd]->m_activeclose) && (evlist->events & EPOLLRDHUP)) 
    {
        printf("events EPOLLRDHUP \n");
        gFDITEM[fd]->m_activeclose = true;
    }

    if (gFDITEM[fd]->m_activeclose)
    {
        timer->remove_timer(&(gFDITEM[fd]));
        gFDITEM[fd]->m_closefun(epollfd, fd, timer);
    }
}

void process_event(int epollfd, struct epoll_event *evlist,
                   TimeValue timeout, TIMER *timers) 
{

    int evlistize = epoll_wait(epollfd, evlist, MAXEPOLLEVENT, timeout);
    if (evlistize > 0) 
    {
        for (int i = 0; i < evlistize; ++i)
        {
            process_one_event(epollfd, evlist + i, timers);
        }
    } 
    else
    {
        // timeout or error
    }

    // perform timed task
    TimeValue timeStamp = getNowTimeStamp();
    while (true)
    {
        void *fditem = timers->get_timer(timeStamp);
        if ( !fditem )
        {
            break;
        }
        FDITEM *p = (FDITEM *)fditem;
        p->m_timeoutfun(epollfd, p->fd, timers, timeStamp);
        //		timers->add
    }
}
