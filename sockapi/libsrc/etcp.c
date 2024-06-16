#include "etcp.h"



void set_address(char *hname, char *sname,
                 struct sockaddr_in *sap, char *protocol)
{
    struct servent *sp;
    struct hostent *hp;
    char *endptr;
    short port;

    bzero(sap, sizeof(*sap));
    sap->sin_family = AF_INET;
    if( hname != NULL )
    {
        if( !inet_aton(hname, &sap->sin_addr) )
        {
            hp = gethostbyname( hname );
            if(hp == NULL)
                error(1, 0, "unknown host: %s\n", hname);
            sap->sin_addr = *(struct in_addr *)hp->h_addr;
        }
    }
    else
        sap->sin_addr.s_addr = htonl( INADDR_ANY );

    port = strtol( sname, &endptr, 0 );
    if( *endptr == '\0' )
        sap->sin_port = htons( port );
    else
    {
        sp = getservbyname( sname, protocol );
        if( sp == NULL )
            error(1, 0, "unknown service: %s\n", sname);
        sap->sin_port = sp->s_port;
    }
}

int readn(int fd, char *buf, size_t len)
{
    ssize_t numRead;
    size_t  totRead;

    for(totRead = 0; totRead < len; )
    {
        numRead = read(fd, buf, len - totRead);
        if( numRead < 0 )               /* read error? */
        {
            if( errno == EINTR )        /* interrupted? */
                continue;               /* restart */
            return -1;                  /* return error */
        }
        if( numRead == 0 )              /* EOF? */
            return totRead;             /* return short len */
        buf += numRead;
        totRead += numRead;
    }
    return totRead;
}

int writen(int fd, const char *buf, size_t len)
{
    ssize_t numWritten;             /* # of bytes written by last write() */
    size_t  totWritten;             /* total # of bytes written so far */

    for(totWritten = 0; totWritten < len; )
    {
        numWritten = write(fd, buf, len - totWritten);
        if(numWritten <= 0)
        {
            if(numWritten == -1 && errno == EINTR)
                continue;           /* Interrupted --> restart write() */
            else
                return -1;          /* Some other error */
        }
        totWritten += numWritten;
        buf += numWritten;
    }
    return totWritten;              /* Must be 'len' bytes if we get here */
}

int readvrec(int fd, char *bp, size_t len)
{
    u_int32_t reclen;
    int rc;

    /* Retrieve the length of the record */
    rc = readn(fd, (char *)&reclen, sizeof(u_int32_t));
    if( rc != sizeof(u_int32_t) )
        return rc < 0 ? -1 : 0;
    reclen = ntohl(reclen);
    if( reclen > len )
    {
        /*
         * Not enough room for the record--
         * discard it and return an error.
         */
        while( reclen > 0 )
        {
            rc = readn(fd, bp, len);
            if( rc != len )
                return rc < 0 ? -1 : 0;
            reclen -= len;
            if( reclen < len )
                len = reclen;
        }
        set_errno( EMSGSIZE );
        return -1;
    }
    /* Retrieve the record itself */
    rc = readn(fd, bp, reclen);
    if( rc != reclen )
        return rc < 0 ? -1 : 0;
    return rc;
}

int tcp_client(char *hname, char *sname)
{
    struct sockaddr_in peer;
    int s = -1;

    set_address(hname, sname, &peer, "tcp");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( connect(s, (struct sockaddr *)&peer, sizeof(peer)) )
        error(1, errno, "connect failed");
    return s;
}

int tcp_server(char *hname, char *sname)
{
    struct sockaddr_in local;
    //struct sockaddr_in peer;
    //int peerlen;
    //int s1;
    int s = -1;
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

int udp_client(char *hname, char *sname,
                  struct sockaddr_in *sap)
{
    int s = -1;

    set_address(hname, sname, sap, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    return s;
}

int udp_server(char *hname, char *sname)
{
    int s = -1;
    struct sockaddr_in local;

    set_address(hname, sname, &local, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if( !isvalidsock(s) )
        error(1, errno, "socket call failed");
    if( bind(s, (struct sockaddr *)&local, sizeof(local)) )
        error(1, errno, "bind failed");
    return s;
}

int readline(int fd, char *bufptr, size_t len)
{
    char *bufx = bufptr;
    char *bp;
    int cnt = 0;
    char b[1500];
    char c;

    while( --len > 0 )  /* To ensure there always have one byte to store '\0' */
    {
        if( --cnt <= 0 )/* To ensure only call <recv> once until finish data copy */
        {
            cnt = recv(fd, b, sizeof(b), 0);
            if(cnt < 0)
            {
                if(errno == EINTR)
                {
                    len++;      /* the while will decrement */
                    continue;
                }
                return -1;
            }
            if(cnt == 0)
                return 0;
            bp = b;
        }
        c = *bp++;
        *bufptr++ = c;
        if( c == '\n' )
        {
            *bufptr = '\0';
            return bufptr - bufx;
        }
    }
    set_errno(EMSGSIZE);
    return -1;
}

int inetConnect(const char *host, const char *service, int type)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;    // Allows IPv4 or IPv6
    hints.ai_socktype = type;

    s = getaddrinfo(host, service, &hints, &result);
    if(s !=  0)
    {
        errno = ENOSYS;
        return -1;
    }

    /* Walk through returned list until we find an address structure
     * that can be used to successfully connect a socket */
    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
        {
            continue;   // On error, try next address
        }

        if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            break;      // Success
        }

        // Connect failde: close this socket and try next address
        close(sfd);
    }
    freeaddrinfo(result);

    return (rp == NULL) ? -1 : sfd;
}

// Public interfaces: inetBind() and inetListen()
static int inetPassiveSocket(const char *service, int type, socklen_t *addrlen, bool doListen, int backlog)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, optval, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;    // Allows IPv4 or IPv6
    hints.ai_socktype = type;
    hints.ai_flags = AI_PASSIVE;    // Use wildcard IP address

    s = getaddrinfo(NULL, service, &hints, &result);
    if(s != 0)
        return -1;

    /* Walk through returned list until we find an address structure
     * that can be used to successfully connect a socket */
    optval = 1;
    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
            continue;

        if(doListen)
        {
            if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, 
                    sizeof(optval)) == -1)
            {
                close(sfd);
                freeaddrinfo(result);
                return -1;
            }
        }

        if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;      // Success

        close(sfd);     // Failed
    }

    if(rp != NULL && doListen)
    {
        if(listen(sfd, backlog) == -1)
        {
            freeaddrinfo(result);
            return -1;
        }
    }

    if(rp != NULL && addrlen != NULL)
        *addrlen = rp->ai_addrlen;  // Return address structure size

    freeaddrinfo(result);

    return (rp == NULL) ? -1 : sfd;
}

int inetListen(const char *service, int backlog, socklen_t *addrlen)
{
    return inetPassiveSocket(service, SOCK_STREAM, addrlen, true, backlog);
}

int inetBind(const char *service, int type, socklen_t *addrlen)
{
    return inetPassiveSocket(service, type, addrlen, false, 0);
}

char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen, char *addrStr, int addrStrLen)
{
    #ifndef _BSD_SOURCE
    #define _BSD_SOURCE     // To get NI_MAXHOST and NI_MAXSERV
                            // definitions from <netdb.h>
    #endif
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if(getnameinfo(addr, addrlen, host, NI_MAXHOST,
                   service, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
        snprintf(addrStr, addrStrLen, "(%s, %s)", host, service);
    else
        snprintf(addrStr, addrStrLen, "(?UNKNOWN?)");

    addrStr[addrStrLen-1] = '\0';   // Ensure result is null-terminated

    return addrStr;
}


