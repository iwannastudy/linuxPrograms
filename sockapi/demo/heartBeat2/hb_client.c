#include "etcp.h"
#include "heartbeat.h"

int main(int argc, char **argv)
{
    fd_set allfd;
    fd_set readfd;
    char msg[1024];
    struct timeval tv;
    struct sockaddr_in hblisten;
    SOCKET sdata;
    SOCKET chb;
    SOCKET slisten;
    int rc;
    int hblistenlen = sizeof(hblisten);
    int heartbeats = 0;
    int maxfd1;
    char hbmsg[1];

    slisten = tcp_server(NULL, "0");
    rc = getsockname(slisten, (struct sockaddr *)&hblisten, 
                                            &hblistenlen);
    if(rc)
        error(1, errno, "getsockname failure");

    sdata = tcp_client(argv[1], argv[2]);
    rc = send(sdata, (char *)&hblisten.sin_port, 
                         sizeof(hblisten.sin_port), 0);
    if(rc < 0)
        error(1, errno, "send failure sending port");

    chb = accept(slisten, NULL, NULL);
    if( !isvalidsock(chb) )
        error(1, errno, "accept failure");

    FD_ZERO(&allfd);
    FD_SET(sdata, &allfd);
    FD_SET(chb, &allfd);
    maxfd1 = (sdata > chb ? sdata : chb) + 1;
    tv.tv_sec = T1;
    tv.tv_usec = 0;

    for(;;)
    {
        readfd = allfd;
        rc = select(maxfd1, &readfd, NULL, NULL, &tv);
        if(rc < 0)
            error(1, errno, "select failure");
        if(rc == 0)     /* timed out */
        {
            if( ++heartbeats > 3 )
                error(1, 0, "connect dead\n");
            error(0, 0, "sending heartbeat #%d\n", heartbeats);
            rc = send(chb, "", 1, 0);
            if(rc < 0)
                error(1, errno, "send failure");
            tv.tv_sec = T2;
            continue;
        }
        
        if(FD_ISSET(chb, &readfd))
        {
            rc = recv(chb, hbmsg, 1, 0);
            if(rc == 0)
                error(1, 0, "server terminnated (chb)\n");
            if(rc < 0)
                error(1, errno, "bad recv on chb");
        }

        if(FD_ISSET(sdata, &readfd))
        {
            rc = recv(chb, msg, sizeof(msg), 0);
            if(rc == 0)
                error(1, 0, "server terminnated (sdata)\n");
            if(rc < 0)
                error(1, errno, "recv failure");
            /* process data */
        }
        heartbeats = 0;
        tv.tv_sec = T1;
    }
}
