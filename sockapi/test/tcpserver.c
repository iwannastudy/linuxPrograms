#include "etcp.h"

void server_run(int cskt, struct sockaddr_in paddr)
{
    ssize_t size;
    char buf[100];
    char clientInfo[NI_MAXHOST+NI_MAXSERV];

    inetAddressStr((struct sockaddr *)&paddr, sizeof(paddr), clientInfo, sizeof(clientInfo));

    printf("Client %d: %s connecting.\n", cskt, clientInfo);

    while(cskt)
    {
        size = recv(cskt, buf, sizeof(buf), 0);
        if(size<0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        printf("recv from client: %s\n", buf);

        size = send(cskt, buf, sizeof(buf), 0);
        if(size<0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }

        
    };
}

int main(int argc, char **argv)
{
    struct sockaddr_in peer;
    int peerlen = 0;
    int client_socket = -1;

    if(argc != 2)
    {
        printf("Please input correct parameters!\n");
        return -1;
    }

    int server_socket = tcp_server(NULL, argv[1]);
    
    while(server_socket)
    {
        peerlen = sizeof(peer);
        client_socket = accept(server_socket, (struct sockaddr *)&peer, &peerlen);
        if( !isvalidsock(client_socket) )
            error(1, errno, "accept failed");

        server_run(client_socket, peer);

        CLOSE(client_socket);
    };
    
    EXIT(0);
}
