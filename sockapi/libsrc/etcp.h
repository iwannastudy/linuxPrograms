#ifndef __ETCP_H__
#define __ETCP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "skel.h"

typedef enum boolean
{
    false = 0,
    true
}bool;

#define NLISTEN 128
void set_address(char *host, char *port, struct sockaddr_in *sap, char *protocol);
int tcp_server(char *host, char *port);
int tcp_client(char *host, char *port);
int udp_server(char *host, char *port);
int udp_client(char *host, char *port, struct sockaddr_in *sap);
int readn(int fd, char *buf, size_t len);
int writen(int fd, const char *buf, size_t len);
int readvrec(int fd, char *buf, size_t len);
int readline(int fd, char *buf, size_t len);
int inetConnect(const char *host, const char *service, int type);
int inetListen(const char *service, int backlog, socklen_t *addrlen);
int inetBind(const char *service, int type, socklen_t *addrlen);
char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen, char *addrStr, int addrStrLen);

#define IS_ADDR_STR_LEN 4096
                        /* suggested length for string buffer that caller
                         * should pass to inetAddressStr(). Must be greater
                         * than (NI_MAXHOST + NI_MAXSERV + 4)*/

#endif
