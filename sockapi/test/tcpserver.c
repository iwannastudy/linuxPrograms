#include "etcp.h"

SOCKET tcp_server(char *hname, char *sname)
{
    struct sockaddr_in local;
    //struct sockaddr_in peer;
    //int peerlen;
    //SOCKET s1;
    SOCKET s;
    const int on = 1;

    set_address(hname, sname, &local, "tcp");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
        error(1, errno, "setsockopt failed");
    if( bind(s, (struct sockaddr *)&local, sizeof(local)) )
        error(1, errno, "bind failed");
    if( listen(s, NLISTEN) )
        error(1, errno, "listen failed");
    return s;

    // do
    // {
    //     peerlen = sizeof(peer);
    //     s1 = accept(s, (struct sockaddr *)&peer, &peerlen);
    //     if( !isvalidsock(s1) )
    //         error(1, errno, "accept failed");
    //     server(s, &peer);
    //     CLOSE(s1);
    // }while(1);
    //
    // EXIT(0);
}
