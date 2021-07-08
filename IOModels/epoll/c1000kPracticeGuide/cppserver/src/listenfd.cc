#include "network.h"

extern PFDITEM gFDITEM[MAX_FD];
extern int sockfdpair[MAX_CPU][2];
struct SendFdBuff sendfdBuff[MAX_CPU];
extern int cpu_num;
extern pthread_mutex_t dispatch_mutex;
extern pthread_cond_t dispatch_cond;

struct Accepted_FD *acceptedFDlist = NULL;

//int movefddata(int fd, char *buff, int len);

int sendConnectionsToWorker(int fd);
int sumaccept = 0;
// listen fd, 当有可读事件发生时调用
int accept_readfun(int epollfd, int listenfd, TIMER *timerlink) 
{
    struct sockaddr_in servaddr;
    int len = sizeof(servaddr);
    int newacceptfd = 0;

    char fdBuff[MAXACCEPTSIZE * 4] = {0};
    int acceptindex = 0;
    int *pbuff = (int *)fdBuff;

    while (true) 
    {
        memset(&servaddr, 0, sizeof(servaddr));
        newacceptfd =
            accept(listenfd, (struct sockaddr *)&servaddr, (socklen_t *)&len);
        if (newacceptfd > 0)
        {   
            // accept new fd
            if (newacceptfd >= MAX_FD)
            {  
                // 太多链接 系统无法接受
                Log << "accept fd  close fd:" << newacceptfd << std::endl;
                close(newacceptfd);
                continue;
            }
            *(pbuff + acceptindex) = newacceptfd;
            acceptindex++;
            if (acceptindex < MAXACCEPTSIZE)
            {
                continue;
            }

            // accept 1k connections, send condtion to dispatch_conn thread.
            struct Accepted_FD *temp = new Accepted_FD;
            temp->acceptedFdLen = acceptindex * 4;
            memcpy(temp->acceptedFdBuff, fdBuff, temp->acceptedFdLen);
            temp->next = NULL;

            pthread_mutex_lock(&dispatch_mutex);
            // add the 1k connection into the accepted fd list.
            if (acceptedFDlist)
            {
                temp->next = acceptedFDlist;
                acceptedFDlist = temp;
            }
            else
            {
                acceptedFDlist = temp;
            }
            pthread_cond_broadcast(&dispatch_cond);
            pthread_mutex_unlock(&dispatch_mutex);
            acceptindex = 0;    // reset to 0 to accept


        } 
        else if (newacceptfd == -1)
        {
            if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO &&
                errno != EINTR)
            {
                Log << "errno ==" << errno << std::endl;
                perror("read error1 errno");
                exit(0);
            }
            else
            {

                if (acceptindex)
                {
                    struct Accepted_FD *temp = new Accepted_FD;
                    temp->acceptedFdLen = acceptindex * 4;
                    memcpy(temp->acceptedFdBuff, fdBuff, temp->acceptedFdLen);
                    temp->next = NULL;

                    pthread_mutex_lock(&dispatch_mutex);
                    if (acceptedFDlist)
                    {
                        temp->next = acceptedFDlist;
                        acceptedFDlist = temp;
                    } 
                    else
                    {
                        acceptedFDlist = temp;
                    }
                    pthread_cond_broadcast(&dispatch_cond);
                    pthread_mutex_unlock(&dispatch_mutex);
                    acceptindex = 0;
                }
                break;
            }
        } 
        else
        {
            Log << "errno 3==" << errno << std::endl;
            exit(1);
            break;
        }
    }
    return 1;
}

int accept_write(int fd) { return 1; }

int fdsend_writefun(int epollfd, int sendfd, TIMER *timerlink)
{
    Log << "call fdsend_writefun" << std::endl;
    return sendConnectionsToWorker(sendfd);
}

// int movefddata(int fd, char *buff, int len)
// {
//
//     memmove(sendfdBuff[fd].buff + sendfdBuff[fd].len, buff, len);
//     sendfdBuff[fd].len += len;
//     return sendfdBuff[fd].len;
// }

int sendConnectionsToWorker(int fd) 
{

    int needSend = sendfdBuff[fd].needSendLen;
    int sended = 0;
    int count = 0;
    char *buff = sendfdBuff[fd].buff;
    while (true) 
    {
        if (needSend - sended > 0) 
        {
            count = send(fd, buff + sended, needSend - sended, 0);
            if (count > 0) 
            {
                sended += count;
            } 
            else 
            {
                // send error occur.
                //assert(false);
                break;
            }
        } 
        else 
        {
            // send completely
            break;
        }
    }
    if (needSend == sended) 
    {
        // LOG("send completely\n");
    }
    else 
    {
        memmove(buff, buff + sended, needSend - sended);
    }
    sendfdBuff[fd].needSendLen = needSend - sended;
    return sended;
}

// Dispatch conections to the worker threads via AF_LOCAL socket.
void *dispatch_conn(void *arg)
{
    static long sum = 0;
    struct Accepted_FD *paccept = NULL;
    for (;;) 
    {
        pthread_mutex_lock(&dispatch_mutex);
        while(!acceptedFDlist)
        {
            pthread_cond_wait(&dispatch_cond, &dispatch_mutex);
        }
        paccept = acceptedFDlist;
        acceptedFDlist = NULL;
        pthread_mutex_unlock(&dispatch_mutex);

        // send the fd from list to worker
        while (paccept) 
        {
            int channel = sum % (cpu_num - 1) + 1;
            int workerPeerfd = sockfdpair[channel][0];
            //movefddata(workerPeerfd, paccept->fdBuff, paccept->len);
            memmove(sendfdBuff[workerPeerfd].buff + sendfdBuff[workerPeerfd].needSendLen, 
                    paccept->acceptedFdBuff, paccept->acceptedFdLen);
            sendfdBuff[workerPeerfd].needSendLen += paccept->acceptedFdLen;
            sendConnectionsToWorker(workerPeerfd);
            ++sum;
            // release the dispatched accepted fd node from list and move to next.
            struct Accepted_FD *next = paccept->next;
            delete paccept;
            paccept = next;
        }
    }
    return NULL;
}
