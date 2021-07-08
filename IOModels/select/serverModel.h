#ifndef __SERVERMODEL_H__
#define __SERVERMODEL_H__
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_PORT                    49152   // private in [49152,65535]
#define BACKLOG_CONNECTIONS         8

#define MAX_CONNECTIONS             10      // num of max Connections
#define NET_BUFFER_SIZE             1024    // bytes
#define LARGE_NET_BUFFER_SIZE       2048

#define NO_BLOCK -1
/* typedef struct NetBuffer */
// {
//     struct NetBuffer *  next;
//     int                 block;  // index in block table of data
//     char                usePool;
//     char *              startAddr;
//     int                 length;
//     char *              data;
/* }NetBuffer, *pNetBuffer; */

typedef enum
{
    eIdle = 0,
    eNewConnection,
    eWaitForRecvData,
    eWaitForSendData,
    eProcessingRequest,
    eDataReceived,
    eDataSending,
    eFinishedRequest,
    eUnrecoverableFailureOrEnd
}ConnectionState;

// This call will be set if some request need a unique handler
typedef bool (*HandleHook)(struct Connection *);

// Structure that will allow us to keep connections open
typedef struct Connection
{
    // Considering the portability of types of variables
    // int  --> uint32
    // long --> uint64
    // ...
    bool                        usb_connection;
    bool                        tcp_sonnection;
    //bool                      auto_ssl;
    //bool                      ClientWantsToUpgradeThisConnectionToSSL
    //bool                      ssl_connection;
    //bool                      xmpp_connection;
    //bool                      ipp_connection;
    //bool                      ipp_firsttime;
    // ...
    bool                        inUse;
    int                         socket;             // connected socket
    bool                        PclosePending;
    struct sockaddr_storage     peer;               // storage IPv4/IPv6 addr
    struct sockaddr_storage     local;
    ConnectionState             state;
    int                         idleTime;
    bool                        waitTickValid;
    int                         waitTickTime;
    //pNetBuffer                pendingSendBuffer;
    //pNetBuffer                partialSendBuffer;
    //pNetBuffer                recvBuffer;
    //可优化为使用时才动态分配内存, 并记录数据长度等数据
    char                        pendingSendBuffer[NET_BUFFER_SIZE];
    char                        partialSendBuffer[NET_BUFFER_SIZE];
    char                        recvBuffer[NET_BUFFER_SIZE];
    /*
     * These 2 variables are to keep track of the data already
     * provided to the http_recv function for the connection.
     */
    //pNetBuffer                trackRecvBuffer;
    //char                      trackRecvBuffer[NET_BUFFER_SIZE];
    //int                       recvNextByteinBuffer;

    HandleHook                  clientCallback;
    int                         clientCallbackStackSize;
    bool                        clientCallbackRunningNow;
    bool                        isCallbackHandlingWrites;
    //bool                      clientParseHeaders;

}Connection, *pConnection;

static void ServerModelMain();
static bool Initialize(int desireMaxConnections);
static int  CreateListeningSocket(short port, unsigned short backlog, bool loopback);
static void NewConnection(int masterSocket);
static void CloseConnection(pConnection connection, bool usbConnection);
static void ReceiveLoop(fd_set *fds);
static void TransmitPendingLoop(fd_set *fds);
static bool TransmitPending(pConnection connection);
static void ServerRun();
static int  HandleConnectionHook(pConnection connection);
static bool SpawnHookThread(pConnection connection);
static HandleHook CommonHook(void *conn);

#endif
