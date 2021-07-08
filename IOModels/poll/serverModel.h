#ifndef __SERVERMODEL_H__
#define __SERVERMODEL_H__
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef DEBUG
#define DEBUG printf
#endif

#ifndef bool
typedef enum BOOL
{
    false = 0,
    FALSE = 0,
    true  = 1,
    TRUE  = 1,
}bool;
#endif

#define MAX_SERVICE                 3
#define PORT1                       50000   // private in [49152,65535]
// Maybe a port 50002/50001 service or other here
#define BACKLOG_CONNECTIONS         8

#define MAX_CONNECTIONS             10      // num of max Connections
#define MAX_WATCHEDFD               MAX_CONNECTIONS+MAX_SERVICE    // SUGGEST less than 100!!!
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
    //eWaitForRecvData,
    //eWaitForSendData,
    //eProcessingRequest,
    eDataReceived,
    //eDataSending,
    //eFinishedRequest,
    //eUnrecoverableFailureOrEnd
}ConnectionState;

/* NOTE: this array must match the order of the HttpConnectionState
   enumeration above... */
static const char* ConnectionStateStrings[] =
{
    "Idle",
    "New Connection Established",
    //"Waiting for Recv Data",
    //"Waiting for Send Data",
    //"Processing Request",
    "Data Received",
    //"Data Sending",
    //"Finished Processing Request",
    //"Unrecoverable Failure or End: Close"
};
// This call will be set if some request need a unique handler
typedef void (*HandleHook)(void *);

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
    bool                        closePending;
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
    size_t                      clientCallbackStackSize;
    bool                        clientCallbackRunningNow;
    bool                        isCallbackHandlingWrites;
    //bool                      clientParseHeaders;

}Connection, *pConnection;

 void ServerModelMain();
 bool Initialize(/*int desireMaxConnections*/);
 int  CreateListeningSocket(short port, unsigned short backlog, bool loopback);
 int NewConnection(int masterSocket);
 void CloseConnection(pConnection connection);
 void ReceiveLoop();
 void TransmitPendingLoop();
 bool TransmitPending(pConnection connection);
 void ServerRun();
 int  HandleConnectionHook(pConnection connection);
 bool SpawnHookThread(pConnection connection);
 HandleHook CommonHook(void *conn);
 int findIndex(int socket);

#endif
