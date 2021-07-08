#include "serverModel.h"
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>

/********************GLOBAL DATA*******************/
pthread_mutex_t mtx;

/**************************************************/

/********************STATIC DATA*******************/
static struct pollfd *watchedfds = NULL;
static pConnection pClients = NULL;
static int connectionsActive = 0;
//static int MAX_CONNECTIONS = 0;
/**************************************************/

bool TransmitPending(pConnection connection)
{
    int retry = 0;
    int count = 0;
    bool status = false;
    int savedErrno = 0;

    if(NULL == connection || connection->socket == -1)
    {
        return status;
    }

    pthread_mutex_lock(&mtx);

    int socket = connection->socket;
    char *buffer = connection->pendingSendBuffer;
    int bytesToBeSend = strlen(buffer) + 1; // Get length of true data + '\n'
    status = true;

    while(bytesToBeSend > 0)
    {
        count = send(socket, buffer, bytesToBeSend, 0);
        if(count > 0)
        {
            buffer += count;
            bytesToBeSend -= count;
        }
        else if(count < 0)  // error
        {
            savedErrno = errno;
            switch(savedErrno)
            {
                case EINTR:
                case EAGAIN: //EWOULDBLOCK
                    {
                        retry++;
                        if(retry > 3)
                        {
                            status = false;
                            break;
                        }
                        else
                            continue;
                    }
                case EPIPE:
                default:
                    status = false;
            }
            DEBUG("error happened, %s\n", strerror(savedErrno));
        }
        else    // count == 0
        {
            DEBUG("client closed.");
            status = false;
        }
    }

    if(bytesToBeSend == 0)
    {
        DEBUG("Had sent all data, clear it.\n");
        memset(connection->pendingSendBuffer, 0, NET_BUFFER_SIZE);
    }

    pthread_mutex_unlock(&mtx);

    DEBUG("returning status %s", ((status == true) ? "OK" : "ERROR"));
    return status;
}

void TransmitPendingLoop()
{
    if( NULL == watchedfds )
    {
        return;
    }

    bool status;

    // Scan for write ready events
    for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if( (true == pClients[i].inUse) &&
            (NULL != pClients[i].pendingSendBuffer) &&
            (watchedfds[findIndex(pClients[i].socket)].fd | POLLOUT) )
        {
            DEBUG("calling TransmitPending for connection[%d]\n", i);
            status = TransmitPending(&pClients[i]);
            if(true != status)
            {
                CloseConnection(&pClients[i]);
            }
            else if(NULL == pClients[i].pendingSendBuffer)  // status == true
            {
                // normal path
                DEBUG("send on %d completed\n", pClients[i].socket);
                if(pClients[i].state != eIdle)
                {
                    pClients[i].state = eIdle;
                    pClients[i].idleTime = 0;
                    DEBUG("responsed the request, set it idle\n");
                }
                if(true == pClients[i].closePending)
                {
                    DEBUG("close-pending TRUE, close connection");
                    CloseConnection(&pClients[i]);
                }
            }
            else
            {
                // should not got here
                // If we've gotten here, there must still be pending buffers to send out
                // stay in the eDataSending state...
                DEBUG("TransmitPending succeeded, but we've still got buffers to send out");
            }
        }
        else if( (false == pClients[i].inUse) &&
                 (NULL != pClients[i].pendingSendBuffer) )
        {
            DEBUG("closeing sockfd = %d. No longer inUse and all buffers are sent.",pClients[i].socket);
            CloseConnection(&pClients[i]);
        }
    }
    return;
}

int findIndex(int socket)
{
    assert(socket);

    int index = -1;
    for( int i=MAX_SERVICE; i<MAX_WATCHEDFD; i++ )
    {
        if(socket == watchedfds[i].fd)
        {
            index = i;
            break;
        }
    }

    assert(index);
    return index;
}

void ReceiveLoop()
{
    int bytes;

    if( (0 == connectionsActive) || (NULL == watchedfds) )
    {
        return;
    }

    DEBUG("handle all the requests for %d active connections\n", connectionsActive);

    for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if( (true == pClients[i].inUse) &&
            (watchedfds[findIndex(pClients[i].socket)].fd | POLLIN) )// this socketfd.revents & POLLIN
        {
            bytes = recv(pClients[i].socket, pClients[i].recvBuffer, NET_BUFFER_SIZE, 0);
            if( bytes > 0 )
            {
                // some handles or not have
                // ...
                pClients[i].state = eDataReceived;
            }
            else
            {
                // client closed the TCP connection
                CloseConnection(&pClients[i]);
            }
        }
    }
    return;
}

void CloseConnection(pConnection connection)
{
    if(NULL == connection || 0 == connectionsActive)
    {
        return;
    }

    // Remove it from watchedfds list
    for(int i=MAX_SERVICE; i<MAX_WATCHEDFD; i++)
    {
        if(connection->socket == watchedfds[i].fd)
        {
            memset(&watchedfds[i], 0, sizeof(struct pollfd));
            watchedfds[i].fd = -1;
            break;
        }
    }

    close(connection->socket);
    memset(connection, 0, sizeof(Connection));
    connection->socket = -1;
    connectionsActive--;
    return;
}


int NewConnection(int serverSocket)
{
    int peerSocket = -1;

#define reserveConn 2

    // Make sure there reserve al least 2 connections for other type of connection
    if(connectionsActive + reserveConn >= MAX_CONNECTIONS)
    {
        // Try removing the longest, unuse network connection to accept the new connection
        int staleIndex = -1;
        int longestIdle = 0;
        for(int i = reserveConn; i < MAX_CONNECTIONS; i++)
        {
            if( (longestIdle < pClients[i].idleTime) &&
                (eIdle == pClients[i].state || eNewConnection == pClients[i].state) &&
                //(false == pClients[i].usb_connection) &&
                //(pClients[i].idleTime > x) &&
                (false == pClients[i].clientCallbackRunningNow))
            {
                staleIndex = i;
                longestIdle = pClients[i].idleTime;
            }
        }

        if(-1 != staleIndex)
        {
            // Found a connection to close in order to make room for the new connections request
            DEBUG("found a connection to close (#%d), idleTime was %d\n",staleIndex, longestIdle);
            CloseConnection(&pClients[staleIndex]);
        }
        else
        {
            DEBUG("did not find a connection to close\n");
            //DumpConnectionList();
            return -1;
        }
    }

    // establish a new connection
    int indexToUse = -1;
    for(int i = reserveConn; i < MAX_CONNECTIONS; i++)
    {
        if(false == pClients[i].inUse)
        {
            indexToUse = i;     // find available one
            break;
        }
    }
    if(-1 != indexToUse)
    {
        int addrlen = sizeof(struct sockaddr_storage);
        peerSocket = accept(serverSocket, (struct sockaddr *)&pClients[indexToUse].peer, &addrlen);
        if(peerSocket >= 0)
        {
            DEBUG("accept a new socket %d\n", peerSocket);
            memset(&pClients[indexToUse], 0, sizeof(struct Connection));

            int addrlen2 = sizeof(struct sockaddr_storage);
            getsockname(peerSocket, (struct sockaddr *)&pClients[indexToUse].local, &addrlen2);

            // Set for non-blocking I/O
            int on = 1;
            ioctl(peerSocket, FIONBIO, (int *)&on);
            //fctnl(peerSocket, F_SETFL, O_NONBLOCK);??? A:also works on socket

            pClients[indexToUse].socket = peerSocket;
            pClients[indexToUse].inUse = true;
            pClients[indexToUse].state = eNewConnection;
            //DEBUG("\n");
            //pClients[indexToUse].usb_connection = usbConnection;
            //pClients[indexToUse].ipp_connection = Is_IPP_Connection;
            // ...
            switch(serverSocket)
            {
                // According different listening socket set different callback
                case PORT1:
                    pClients[indexToUse].clientCallback = PORT1_callback;
                    pClients[indexToUse].clientCallbackStackSize = 4096;
                    break;
                //case PORT2:
                //default:
            }

            // new client connected, watch it
            for(int i=MAX_SERVICE; i < MAX_WATCHEDFD; i++)
            {
                if(watchedfds[i].fd == -1)   // find a available area to watch this socket;
                {
                    watchedfds[i].fd = peerSocket;
                    watchedfds[i].events = POLLIN | POLLOUT;    // check POLLRDHUP->peer closed
                    break;
                }
            }
            //TODO:是否要在这里建立两个数组之间的索引?
            connectionsActive++;
        }
        else
        {
            DEBUG("ERROR: accept() failed on socket %d\n", serverSocket);
            //??? if accept error need exit?
            //assert(false);
        }
    }
    else
    {
        DEBUG("unable to accept new connection...already maxed out\n");
    }
    return peerSocket;
}

int CreateListeningSocket(short port, unsigned short backlog, bool loopback)
{
    int sockfd = -1;
    int ret;
    const int len = 32768;  // 32k
    struct addrinfo *res, *ressave;
    static char Serverservice[10];
    struct addrinfo hints;
    char *serverhost;
    int hintsFlags;

    serverhost = NULL; // NULL时，得到一个通配地址或本地回环地址，取决于AI_PASSIVE的设置与否
    hintsFlags = AI_NUMERICSERV;    // 将service解释成一个数值端口号，以防止调用任何dns
    hintsFlags |= loopback ? 0 : AI_PASSIVE;    // 0 --> loopback address
                                                // AI_PASSIVE --> listen socket address

    sprintf(Serverservice, "%d", port);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = hintsFlags;
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;// stream socket

    // getaddrinfo() handles IPv4/IPv6 address transparently.
    ret = getaddrinfo(serverhost, Serverservice, &hints, &res);
    if(ret != 0)
    {
        DEBUG("ERROR: getaddrinfo failure; error = %s\n", gai_strerror(ret));
        assert(false);
    }

    ressave = res;
    while(res)
    {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sockfd >= 0)
        {
            if(bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            {
                DEBUG("Successful bind(); res->ai_family=%d, res->ai_socktype=%d, res->ai_protocol=%d",
                res->ai_family, res->ai_socktype, res->ai_protocol);
                listen(sockfd, backlog);
                break;
            }
            close(sockfd);
            sockfd = -1;
        }
        res = res->ai_next;
    }
    freeaddrinfo(ressave);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&len, sizeof(int));
    DEBUG("setsockopt() operation result = %d\n", ret);

    return sockfd;
}

bool SpawnHookThread(pConnection connection)
{
    void *hook = (void *)connection->clientCallback;

    if(!connection || connection->socket == -1)
    {
        assert(false);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    size_t stacksize = connection->clientCallbackStackSize;
    pthread_attr_setstacksize(&attr, stacksize);
    // set priority for this thread
    // pthread_attr_setschedparam();

    pthread_t pid;
    int status = pthread_create(&pid, &attr, hook, (void *)connection);
    assert(status); 

    pthread_attr_destroy(&attr);
    return true;
}


int HandleConnectionHook(pConnection connection)
{
    bool status;
    //DEBUG("");

    // LOCK

    if(!connection->clientCallbackStackSize)
    {
        connection->clientCallbackStackSize = 2048; // 2k as default
    }

    DEBUG("calling callback for client handler (on its own thread).");
    if( !SpawnHookThread(connection) )
    {
        connection->clientCallbackRunningNow = false;
        status = false;
    }
    else
    {
        connection->clientCallbackRunningNow = true;
        status = true;
    }

    // UNLOCK
    return status;
}

/********************************************************************
 * Handle recvived data and determine what data should be sent.     *
 * And some other handles.                                          *
 ********************************************************************/
void ServerRun()
{
    bool result;

    for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (pClients[i].state)
        {
            DEBUG("client #%d(%d) --> inUse: %d, idleTime: %d, state: '%s'\n",
                        i, pClients[i].socket,
                        pClients[i].inUse, pClients[i].idleTime,
                        ConnectionStateStrings[pClients[i].state]);
        }

        if( (true == pClients[i].inUse) && (pClients[i].clientCallbackRunningNow) )
        {
            // As long as the downstream subsystem is still processing the socket's
            // input stream we'll want to make sure that we don't allow the connection's
            // timestamp to suggest a stale socket in need of a forced close; so keep
            // it invalidate here. Eventually the hook routine will finish and then it
            // will get its first timestamp and then this routine can start monitoring
            // the connection activity to see when/if a forced termination is necessary.
            pClients[i].waitTickValid = false;
        }

        if(true == pClients[i].inUse)
        {
            // check the idle state
            if( (eIdle == pClients[i].state) ||
                (eNewConnection == pClients[i].state) )
            {
                pClients[i].idleTime++;
            }
            else
            {
                pClients[i].idleTime = 0;
            }

            ConnectionState origState;

            // Run this state machine for each connection to ensure that
            // proper responses are returned in a timely manner...
            do
            {
                origState = pClients[i].state;

                switch(pClients[i].state)
                {
                    case eDataReceived:
                    {
                        DEBUG("Processing eDataReceived: i=%d sockfd=%d\n",
                               i, pClients[i].socket);
                        if(pClients[i].clientCallbackRunningNow)
                        {
                            DEBUG("Let callback that is running for socket %d handle the request.\n", pClients[i].socket);
                            break;
                        }

                        DEBUG("handle the request on socket %d", pClients[i].socket);
                        pClients[i].waitTickValid = false;

                        if( NULL != pClients[i].clientCallback )
                        {
                            DEBUG("client callback is set, send data to client\n");

                            // According to the received data to Determine different Client Requests,
                            // then set different callback function, and last spawn a new thread to
                            // run the callback.
                            //result = DetermineClientRequest();
                            //if( result )
                            //{
                                result = HandleConnectionHook(&pClients[i]);
                            //}
                        }
                        else
                        {
                            DEBUG("Should handle the request by myself");
                            //result = HandleRecvData(&pClients[i]);
                        }
                    }
                    break;

                    case eDataSending:
                    {
                        DEBUG("Processing eDataSending: i=%d socket=%d",
                               i, pClients[i].socket);

                        if(pClients[i].clientCallbackRunningNow)
                        {
                            break;
                        }
                        // check to see if there is a partially filled buffer that 
                        // needs to be sent
                        if(NULL != pClients[i].partialSendBuffer)
                        {

                        }
                    }
                    break;

                    case eWaitForRecvData:
                    {
                        if (pClients[i].clientCallbackRunningNow)
                        {
                            break;
                        }
                        if (true == pClients[i].waitTickValid)
                        {
                            int currentTickTime = bTicks2Time(clock_gettick());
                            int diffTime = ((currentTickTime > pClients[i].waitTickTime) ?
                                    (currentTickTime - pClients[i].waitTickTime) : 0);
                            /* check to see if connection needs to be closed...? */
                            if (MAX_WAITING_HTTP_CLIENT < diffTime)
                            {
                                //DEBUG("closing connection, initial: %d...current: %d...diff: %d",
                                //        pClients[i].waitTickTime, currentTickTime, diffTime);
                                // close the connection if we are stale "too long"
                                // (i.e. stuck in waiting for receive data)
                                CloseConnection(&pClients[i]);
                            }
                            else if (0 == diffTime)
                            {
                                /* tick time rolled, update the tick time */
                                pClients[i].waitTickTime = currentTickTime;
                            }
                        }
                        else
                        {
                            pClients[i].waitTickTime  = bTicks2Time(clock_gettick());
                            pClients[i].waitTickValid = true;
                        }
                    }
                    break;

                    //case other:

                     case eUnrecoverableFailureOrEnd:
                        {
                            DEBUG("Processing eUnrecoverableFailureOrEnd: i=%d sockfd=%d",
                                   i, pClients[i].socket);

                            if (HTTP_NET_BUFFER_NULL != pClients[i].partial_send_buffer)
                            {
                                httpAttachSendDataToConnection(&pClients[i],
                                                               pClients[i].partial_send_buffer);
                                pClients[i].partial_send_buffer = HTTP_NET_BUFFER_NULL;
                            }

                            /* transmit the buffer(s) that were just sent... */
                            HRESULT status = TransmitPending(&pClients[i]);
                            DEBUG("TransmitPending() returns %s",
                                        ((status == S_OK) ? "OK" : "ERROR"));

                            /* unrecoverable failure, close the connection */
                            DEBUG("Unrecoverable error on socket %d, close connection",
                                        soap->socket);
                            httpCloseConnection(&pClients[i]);
                        }
                        break;

                    case eProcessingRequest:
                        {
                            DEBUG("Processing eProcessingRequest: i=%d sockfd=%d",
                                   i, pClients[i].socket);

                            if (pClients[i].clientCallbackRunningNow)
                            {
                                break;
                            }
                            pClients[i].waitTickValid = false;

                            soap->error = DetermineClientForRequest(soap, &pClients[i]);

                            if (soap->error == SOAP_OK)
                            {
                                soap->error = httpHandleConnectionHook(&pClients[i]);
                            }
                        }
                        break;

                    case eIdle:
                    default:
                        pClients[i].waitTickValid = false;
                }

            }while(origState != pClients[i].state)
        }
    }
    return;
}

bool Initialize(/*int desireMaxConnections*/)
{
    pthread_mutexattr_t mtxattr;
    int ret;

    // Create a mutex
    pthread_mutexattr_init(&mtxattr);
    pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE);    //TODO:figure out what is mutex attribute
    ret = pthread_mutex_init(&mtx, &mtxattr);
    assert(ret == 0);

    connectionsActive = 0;
    //MAX_CONNECTIONS = (desireMaxConnections > 0 ? desireMaxConnections : MAX_CONNECTIONS);
    //DEBUG("max Connections = %d\n", MAX_CONNECTIONS);

    // Allocate the memory for watched fd
    watchedfds = (struct pollfd *)malloc(sizeof(struct pollfd) * MAX_WATCHEDFD);
    if(watchedfds != NULL)
    {
        memset(watchedfds, 0, sizeof(struct pollfd) * MAX_WATCHEDFD);
        for(int i=0; i<MAX_WATCHEDFD; i++)
        {
            watchedfds[i].fd = -1;
        }
    }
    else
    {
        DEBUG("malloc error\n");
        assert(pClients);
    }

    // Allocate the memory to handle the open connecttions
    pClients = (pConnection)malloc(sizeof(Connection) * MAX_CONNECTIONS);
    if(NULL != pClients)
    {
        memset(pClients, 0, sizeof(Connection) * MAX_CONNECTIONS);
        for(int i = 0; i < MAX_CONNECTIONS; i++)
        {
            pClients[i].socket = -1;
        }
    }
    else
    {
        DEBUG("malloc error\n");
        assert(pClients);
    }

    return true;
}

void ServerModelMain()
{
    // There should declare some socket fds which are used for listening connecttion
    int tcpListeningfd = -1;    // listening socket tcp

    int status;
    //bool needWrite;
    int timeout = 200;   //200 ms
    //nfds_t nfds;


    // Set thread priority
    // TODO

    if( !Initialize(MAX_CONNECTIONS) )
    {
        DEBUG("unable to init server!\n");
        assert(false);
    }
    DEBUG("server variables Initialized\n");

    if(tcpListeningfd == -1)
    {
        // establish listening sockets on the port to check new connection
        tcpListeningfd = CreateListeningSocket(PORT1, BACKLOG_CONNECTIONS, false);
        if(tcpListeningfd != -1)
        {
            // the first one to watch this listening socket - port 50000;
            watchedfds[0].fd = tcpListeningfd;
            watchedfds[0].events = POLLIN;
        }
    }

    while( 1 )
    {
        // Main functions : Run server for outstanding connections
        ServerRun();

        //needWrite = false;

        DEBUG("start poll()\n");
        status = poll(watchedfds, MAX_WATCHEDFD, timeout);
        if(status >= 0)
        {

            //是否轮询POLLHUP

            if(tcpListeningfd != -1)    // in listening
            {
                if( (watchedfds[0].revents & POLLIN) &&
                    (connectionsActive < MAX_CONNECTIONS) )
                {
                    // establish connection for listening socket
                    NewConnection(tcpListeningfd);                    
                }
            }

            // Check the read and write fd for data ready to read
            // or connections ready to accept (write)
            ReceiveLoop();

            //if(needWrite)
            //{
                TransmitPendingLoop();
            //}
        }
        else
        {
            DEBUG("ERROR: poll()\n");
            assert(false);
        }

        // Need more thought about this
        // should be samll value
        //usleep(x); // sleep 10ms
    }
    return;
}
