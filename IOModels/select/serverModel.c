#include "serverModel.h"
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/********************GLOBAL DATA*******************/
pthread_mutex_t mtx;

/**************************************************/

/********************STATIC DATA*******************/
static pConnection pClients = NULL;
static int connectionsActive = 0;
static int maxConnections = 0;
/**************************************************/

static bool TransmitPending(pConnection connection)
{
    char * buffer = NULL;
    char * nextBuffer = NULL;
    static int bytes;
    bool status = false;
    bool ok;

    if(NULL == connection)
    {
        return status;
    }

    pthread_mutex_lock(&mtx);

    status = true;

    int savedErrno;

    // Follow chain of buffers, outputting the contents of each.
    for( (ok = true, buffer = connection->pendingSendBuffer);
            (ok) && (buffer != NULL);
            buffer = nextBuffer )
    {
        int bytesToBeSend = strlen(buffer) + 1; // Get length of true data + '\n'

        // Write buffer data to TCP/IP connection
        errno = 0;
        //DEBUG("sending %d on %d\n", bytesToBeSend, connection->socket);

        if( (connection->socket == -1) )
        {
            bytes = -1;
            savedErrno = EINVAL;
            //DEBUG("Attempting send to close socket\n");
        }
        else
        {
            bytes = send(connection->socket, buffer, bytesToBeSend, 0);
            savedErrno = errno;
        }

        //DEBUG("sent %d on %d\n", bytes, connection->socket);

        // Test for data being successful sent and if so, update buffer
        // pointers to record it
        if(bytes > 0)
        {
            // Some data was sent, so adjust the buffer's pointers to record that fact.
            buffer = buffer + bytes;
            // update buffer length
            // ...
        }
        // If the entire buffer was sent, free it from the buffer chain.
        // (Note this correctly handles the unexpected case where
        // buffer length == 0 -- bytes will == 0, and the buffer will be
        // removed from chain.)
        // if an error was encountered and bytes == -1, then this condition
        // will never be true, which is safe.
        if(strlen(buffer) == 0)
        {
            //DEBUG("buffer is now empty so free it and move to the next buffer.\n");
            // The entire contents of this buffer have been sent, so it should
            // be freed.
            // 'nextBuffer' is the next buffer to be examined in the encloseing
            // for() loop, so we have to set it now, before we free 'buffer'.
            memset(connection->pendingSendBuffer, 0, NET_BUFFER_SIZE);
            buffer = NULL;
        }
        else
        {
            //DEBUG("buffer is not empty so try to send the rest.");
            //Not all of this buffer was accepted by send().
            //'nextBuffer' is set to be 'buffer', so the next iteration of
            //the loop will attempt to send the remainder of the data.
            //(Except if EWOULDBLOCK (flow control) has been indicated, which
            //will be tested for below, and will cause the loop to be exited.)
            nextBuffer = buffer;
        }

        // Test for error conditions and flow control and respond appropriately.

        // check if the connection has been removed while looping through the
        // buffers to send
        if( (NULL == connection) 
                //|| (connection->usb_connection && false == USBCableConnected()) 
          )
        {
            //DEBUG("Connection removed while looping to send");
            // Exit the for().
            ok = false;
            status = false;
            if(buffer != NULL)
            {
                // free
                memset(connection->pendingSendBuffer, 0, NET_BUFFER_SIZE);
                nextBuffer = buffer = NULL;
            }
            break;
        }

        if(bytes < 0)
        {
            switch(savedErrno)
            {
                case 0:
                    // No error seen.
                    //DEBUG("errno = 0, no errors\n");
                    break;
                case EWOULDBLOCK:
                    //DEBUG("errno = EWOULDBLOCK, send on %d flow control on\n", connection->socket);
                    ok = false;
                    status = true;
                    break;
                case EPIPE:
                    //DEBUG("connection dropped 0x%x\n", connection);
                    // fall through to abort connection
                default:
                    //DEBUG("Data send on socket %d failed (errno=%d, bytes=%d)\n", 
                    //connection->socket, savedErrno, bytes);
                    status = false;
                    ok = false;
                    /*
                     * [ASI Support #4054] fix: EPIPE and other error codes from
                     * write previously did not clear any transmit pending condition.
                     * But future attempts to write will also fail, and we will not
                     * close the socket until condition is cleared.  The bug caused
                     * an infinite loop (or worse, a crash), because the socket
                     * was always selected but could only generate error events.  Now
                     * we clear the transmit pending condition by releasing any
                     * queued buffers and setting connection->pending_send_buffers to
                     * HTTP_NET_BUFFER_NULL.  This forces a socket close when the
                     * context is aborted.
                     */
                    if (buffer != NULL)
                    {
                        //DEBUG("Setting buffer to NULL\n");
                        memset(connection->pendingSendBuffer, 0, NET_BUFFER_SIZE);
                        next_buffer = buffer = NULL;
                    }
            }
        }
    }
    /*
     * buffer will be (! NULL) on a EWOULDBLOCK "error" otherwise
     * should be NULL (either all buffers transmitted sucessfully
     * or got an error condition and all buffers were freed).
     */
    //DEBUG("connection->pendingSendBuffer is now 0x%x", buffer);
    //connection->pendingSendBuffer = buffer;

    pthread_mutex_unlock(&mtx);

    //DEBUG("returning status %s", ((status == true) ? "OK" : "ERROR"));
    return status;
}

static void TransmitPendingLoop(fd_set *fds)
{
    if( NULL == fds )
    {
        return;
    }

    bool status;

    // Scan for write ready events
    for(int i = 0; i < maxConnections; i++)
    {
        if( (true == pClients.[i].inUse) &&
            (NULL != pClients[i].pendingSendBuffer) &&
            (FD_ISSET(pClients[i].socket, fds)) )
        {
            //DEBUG("calling TransmitPending for connection[%d]\n", i);
            status = TransmitPending(&pClients[i]);
            if(true != status)
            {
                CloseConnection(&pClients[i]);
            }
            else if(NULL == pClients[i].pendingSendBuffer)
            {
                //DEBUG("send on %d completed\n", pClients[i].socket);
                if(pClients[i].state != eIdle)
                {
                    pClients[i].state = eIdle;
                    pClients[i].idleTime = 0;
                    //DEBUG("\n")
                }
                if(true == pClient[i].closePending)
                {
                    //DEBUG("close-pending TRUE, close connection");
                    CloseConnection(&pClients[i]);
                }
            }
            else
            {
                // If we've gotten here there must still be pending buffers to send out
                // stay in the eDataSending state...
                //DEBUG("TransmitPending succeeded, but we've still got buffers to send out");
            }
        }
        else if( (false == pClients[i].inUse) &&
                 (NULL != pClients.pendingSendBuffer) )
        {
            //DEBUG("closeing sockfd = %d. No longer inUse and all buffers are sent.")
            CloseConnection(&pClients[i]);
        }
    }
    return;
}

static void ReceiveLoop(fd_set *fds)
{
    if( (0 == connectionsActive) || (NULL == fds) )
    {
        return;
    }

    int bytes;

    //DEBUG("handle all the requests for %d active connections\n", connectionsActive);

    for(int i = 0; i < maxConnections; i++)
    {
        if( (-1 != pClients[i].socket) &&
            (true == pClients[i].inUse) &&
            FD_ISSET(pClients[i].socket, fds) )
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

static void CloseConnection(pConnection connection)
{
    if(NULL == connection)
    {
        return;
    }

    if(0 == connectionsActive)
    {
        return;
    }

    close(connections->socket);
    //如果含有动态内存被分配，要分别释放
    memset(connection, 0, sizeof(Connection));
    connection->socket = -1;
    connectionsActive--;
    return;
}
static void NewConnection(int masterSocket, bool usbConnection)
{
#define reserveConn 4
    // if(true == usbConnection)
    // {
    //     if(USBConnectionsPresent() >= NUM_USB_CLIENT_END)
    //     {
    //         return;
    //     }
    // }

    // Make sure there reserve al least 2 connections for other type of connection
    if(connectionsActive + reserveConn >= maxConnections)
    {
        // Try removing the longest, unuse network connection to accept the new connection
        int staleIndex = -1;
        int longestIdle = 0;
        for(int i = reserveConn; i < maxConnections; i++)
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
            //DEBUG("found a connection to close (#%d), idleTime was %d\n",
            //staleIndex, longestIdle);
            CloseConnection(&pClients[staleIndex]);
        }
        else
        {
            //DEBUG("did not find a connection to close\n");
            //DumpConnectionList();
            return;
        }
    }

    // establish a new connection
    int indexToUse = -1;
    for(int i = reserveConn; i < maxConnections; i++)
    {
        if(false == pClients[i].inUse)
        {
            indexToUse = i;
            break;
        }
    }
    if(-1 != indexToUse)
    {
        int addrlen = sizeof(struct sockaddr_storage);
        int thisSocket = accept(masterSocket, (struct sockaddr *)&pClients[indexToUse].peer, &addrlen);
        if(thisSocket >= 0)
        {
            //DEBUG("accept a new socket %d\n", thisSocket);
            memset(&pClients[indexToUse], 0, sizeof(struct Connection));

            int addrlen2 = sizeof(struct sockaddr_storage);
            getsockname(thisSocket, (struct sockaddr *)&pClients[indexToUse].local, &addrlen2);

            // Set for non-blocking I/O
            int on = 1;
            ioctl(thisSocket, FIONBIO, (int *)&on);

            pClients[indexToUse].socket = thisSocket;
            pClients[indexToUse].inUse = true;
            pClients[indexToUse].state = eNewConnection;
            //DEBUG("\n");
            pClients[indexToUse].usb_connection = usbConnection;
            //pClients[indexToUse].ipp_connection = Is_IPP_Connection;
            // ...
            //if(masterSocket == usbSocket0)
            //{
            //    // According different listening socket set different callback
            //    pClients[indexToUse].callback = TODO
            //    pClients[indexToUse].clientCallbackStackSize = TODO
            //}
            connectionsActive++;
        }
        else
        {
            //DEBUG("ERROR: accept() failed\n");
            //??? if accept error need exit?
            //assert(false);
        }
    }
    else
    {
        //DEBUG("unable to accept new connection...already maxed out\n");
    }
    return;
}

static int CreateListeningSocket(short port, unsigned short backlog, bool loopback)
{
    int sockfd = -1;
    int ret;
    const int len = 32768;
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
        //DEBUG("ERROR: getaddrinfo failure; error = %s\n", gai_strerror(ret));
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
                //DEBUG("Successful bind(); res->ai_family=%d, res->ai_socktype=%d, res->ai_protocol=%d",
                //res->ai_family, res->ai_socktype, res->ai_protocol);
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
    //DEBUG("setsockopt() operation result = %d\n", result);

    return sockfd;
}

static HttpHook httpHook(void* conn)
{
    if (!conn) // Sanity Check.
    {
        DEBUG("Error: Check Logic. Should not be possible to get here.");
        assert(false);
    }
    
    pConnection connection = (pConnection)conn;
    
    DEBUG("Running %s for Connection = 0x%X", pthread_getname(pthread_self()), connection);
    DEBUG("calling callback for client handler");
    
    HandleHook hook;
    hook = connection->clientCallback;
    if (hook) // sanity check.
    {
        // Next, now transfer control to the registered hook routine to process the remote host data.
        bool status = hook(connection);
        if (true != status)
        {
            DEBUG("URL that failed: %s", connection->http.requestURI);
            DEBUG("Error: Hook of %s return HRESULT=%d", pthread_getname(pthread_self()), status);
        }
        connection->clientCallbackRunningNow = false;
        DEBUG("%d = inUse", connection->inUse);
        DEBUG("%d = state", connection->state);
        DEBUG("%d = socket", connection->socket);
        DEBUG("%d = pending send buffers", connection->pendingSendBuffers);
        DEBUG("%d = partial send buffer", connection->partial_send_buffer);
        DEBUG("%d = closepending", connection->close_pending);
        DEBUG("%s is terminating", pthread_getname(pthread_self()));
    }
    pthread_exit(NULL);
    return NULL;
}

static bool SpawnHookThread(pConnection connection)
{
    if( !connection )
    {
        assert(false);
    }

    // Derive a thread to process the hook routine on. This thread will
    // have a unique name which is derived from the sockfd
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    int stackSize = connection->clientCallbackStackSize;
    pthread_attr_setstacksize(&attr, (size_t)stackSize);

    // Set thread priority
    // pthread_attr_setschedparm(&attr, ...);

    pthread_t pid;
    DEBUG("Spawing thread (stack size = %d) for connection = 0x%X", stackSize, connection);
    // TODO, CommonHook()
    bool result = pthread_create(&pid, &attr, (void *)&CommonHook, (void *)connection);
    assert(result);

    pthread_attr_destory(&attr);
    return true
}

static int HandleConnectionHook(pConnection connection)
{
    //DEBUG("");
    
    // LOCK

    if(connection->clientCallbackStackSize)
    {
        // Recall that if the client wants their Hook called own a seperate thread
        // then they will have previously supplied a non-zero ThreadStatickSize at
        // the time they registered the hook.
        DEBUG("calling callback for client handler (on its own thread).");

        connection->clientCallbackRunningNow = true;
        if( !SpawnHookThread(connection) )
        {
            connection->clientCallbackRunningNow = false;
            // UNLOCK
            return false;
        }
        else
        {
            //UNLOCK
            return true;
        }
    }
    else
    {
        DEBUG("calling\n");
        HandleHook hook;
        hook = connection->clientCallback;
        bool result = hook(connection);
        if(true != result)
        {
            // UNLOCK
            return flase;
        }
    }

    // UNLOCK
    return true;
}

/********************************************************************
 * Handle recvived data and determine what data should be sent.     *
 * And some other handles.                                          *
 ********************************************************************/
static void ServerRun()
{
    bool result;

    for(int i = 0; i < maxConnections; i++)
    {
        if (pClients[i].state)
        {
            //DEBUG("client #%d(%d) --> inUse: %d, idleTime: %d, state: '%s'",
            //            i, pClients[i].socket,
            //            pClients[i].inUse, pClients[i].idleTime,
            //            HttpConnectionStateStrings[pClients[i].state]);
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
                        //DEBUG("Processing eDataReceived: i=%d sockfd=%d",
                        //i, pClients[i].socket);
                        if(pClients[i].clientCallbackRunningNow)
                        {
                            //DEBUG("Let callback that is running for socket %d handle the request.\n");
                            break;
                        }
                        //DEBUG("handle the request on socket %d", pClients[i].socket);
                        pClients[i].waitTickValid = false;

                        if( NULL != pClients[i].clientCallback )
                        {
                            //DEBUG("client callback set, send data on to client\n");

                            // According to the received data to Determine different Client Requests,
                            // then set different callback function, and last spawn a new thread to
                            // run the callback.
                            result = DetermineClientRequest();
                            if( result )
                            {
                                result = HandleConnectionHook(&pClients[i]);
                            }
                        }
                        else
                        {
                            result = HandleConnectionHook(&pClients[i]);
                        }
                    }
                    break;

                    case eDataSending:
                    {
                        //DEBUG("Processing eDataSending: i=%d socket=%d",
                        //i, pClients[i].socket);

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
                            CHECKPOINTB("Processing eUnrecoverableFailureOrEnd: i=%d sockfd=%d",
                                   i, pClients[i].socket);

                            if (HTTP_NET_BUFFER_NULL != pClients[i].partial_send_buffer)
                            {
                                httpAttachSendDataToConnection(&pClients[i],
                                                               pClients[i].partial_send_buffer);
                                pClients[i].partial_send_buffer = HTTP_NET_BUFFER_NULL;
                            }

                            /* transmit the buffer(s) that were just sent... */
                            HRESULT status = TransmitPending(&pClients[i]);
                            CHECKPOINTB("TransmitPending() returns %s",
                                        ((status == S_OK) ? "OK" : "ERROR"));

                            /* unrecoverable failure, close the connection */
                            CHECKPOINTB("Unrecoverable error on socket %d, close connection",
                                        soap->socket);
                            httpCloseConnection(&pClients[i]);
                        }
                        break;

                    case eProcessingRequest:
                        {
                            CHECKPOINTB("Processing eProcessingRequest: i=%d sockfd=%d",
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

static bool Initialize(int desireMaxConnections)
{
   pthread_mutexattr_t mtxattr;
   int ret;

   // Create a mutex
   pthread_mutexattr_init(&mtxattr);
   pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE);    //TODO:figure out what is mutex attribute
   ret = pthread_mutex_init(&mtx, &mtxattr);
   assert(ret == 0);

   connectionsActive = 0;
   maxConnections = (desireMaxConnections > 0 ? desireMaxConnections : MAX_CONNECTIONS);
   //DEBUG("max Connections = %d\n", maxConnections);
   
   // Allocate the memory to handle the open connecttions
   pClients = (pConnection)malloc(sizeof(Connection) * maxConnections);
   if(NULL != pClients)
   {
       memset(pClients, 0, sizeof(Connection) * maxConnections);
       for(int i = 0; i < maxConnections; i++)
       {
           pClients[i].socket = -1;
       }
   }
   else
   {
       //DEBUG("malloc error\n");
       assert(pClients);
   }

   return true;
}

static void ServerModelMain()
{
    // There should declare some socket fds which are used for listening connecttion
    int tcpSocket = -1;    // listening socket tcp
    // ...
    int usbSocket0 = -1;    // listening socket usb
    int usbSocket1 = -1;
    // ...
    int alttcpSocket = -1;  // listening socket alternate tcp

    int maxSockfd;
    int sockStat;
    bool needWrite;

    struct timeval timeout;
    fd_set readReadyFdset;
    fd_set writeReadyFdset;

    // Set thread priority
    // TODO

    if( !Initialize(MAX_CONNECTIONS) )
    {
        //DEBUG("unable to init server!\n");
        assert(false);
    }
    //DEBUG("server variables Initialized");

    while( 1 )
    {
        // Main functions : Run server for outstanding connections
        ServerRun();

        // Set up fdset lists for the select
        FD_ZERO(&readReadyFdset);
        FD_ZERO(&writeReadyFdset);
        
        needWrite = false;
        maxSockfd = 0;

        // if( usbSocket0 < 0 )
        // {
        //     // establish the socket for the usb connecttion
        //     usbSocket0 = CreateListeningSocket(USBPORT0);
        //     DEBUG("usb socket 0 is %d\n", usbSocket0);
        // }
        // if ((usbSocket0 >= 0) &&
        //         (true == httpHaveAvailableUSBConnection()))
        // {
        //     /* set up a read on the master "USB" socket */
        //     FD_SET(ledmUsbSocket_0, &readReadyFdset);
        //     if (maxSocket < usbSocket0)
        //     {
        //         maxSocket = usbSocket0;
        //     }
        // }

        if(tcpSocket < 0)
        {
            // establish listening sockets on the port to check new connection
            tcpSocket = CreateListeningSocket(TCP_PORT, BACKLOG_CONNECTIONS, false);
            //DEBUG("tcp socket on port %d is %d", TCP_PORT, tcpSocket);
        }
        if(tcpSocket >= 0)
        {
            FD_SET(tcpSocket, &readReadyFdset);
            if(maxSocket < tcpSocket)
            {
                maxSocket = tcpSocket;
            }
        }

        // ... Create other listening socket fds

        for(int i = 0; i < maxConnections; i++)
        {
            if(true = pClients[i].inUse)
            {
                // Add connected socket into read fdset
                int thisSocket = pClients[i].socket;

                if(!pClients[i].clientCallbackRunningNow)   // If the socket is not in handling
                {
                    FD_SET(thisSocket, &readReadyFdset);
                    if(maxSocket < thisSocket)
                    {
                        maxSockfd = thisSocket;
                    }
                }

                if( !(pClients[i].clientCallbackRunningNow && pClients[i].isCallbackHandlingWrites) )
                {
                    if(NULL != pClients[i].pendingSendBuffer)
                    {
                        // Send data in Pending
                        FD_SET(thisSocket, &writeReadyFdset);
                        if(maxSocket < thisSocket)
                        {
                            maxSockfd = thisSocket;
                        }
                    }

                    needWrite = true;
                }
            }
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        //DEBUG("start select()\n");
        sockStat = select(maxSockfd+1, &readReadyFdset, &writeReadyFdset, NULL, &timeout);
        if(sockStat >= 0)
        {
            // if (usbSocket0 >= 0)
            // {
            //     /* new usb connection request, add it the list... */
            //     if ((true == httpHaveAvailableUSBConnection()) &&
            //             (FD_ISSET(usbSocket0, &readReadyFdset)))
            //     {
            //         httpNewConnection(usbSocket0, true);
            //     }
            // }

            if(tcpSocket >= 0)
            {
                if( FD_ISSET(tcpSocket, &readReadyFdset) &&
                    (connectionsActive < maxConnections) )
                {
                    NewConnection(tcpSocket, false);    // establish connection for listening socket
                }
            }
            // Add other listening fd

            // Check the read and write fd for data ready to read
            // or connections ready to accept (write)
            ReceiveLoop(&readReadyFdset);

            if(needWrite)
            {
                TransmitPendingLoop(&writeReadyFdset);
            }
        }
        else
        {
            //DEBUG("ERROR: select()\n");
            assert(false);
        }

        // Need more thought about this
        // should be samll value
        //usleep(x); // sleep 10ms
    }
    return;
}
