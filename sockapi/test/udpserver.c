#include "etcp.h"



int main(int argc, char **argv)
{
    struct sockaddr_in peer;
    socklen_t caddrlen = 0;
    char buf[100];
    ssize_t size;

    if(argc != 2)
    {
        printf("Please input correct parameters!\n");
        return -1;
    }

    int server_socket = udp_server(NULL, argv[1]);
    
    while(server_socket)
    {
        caddrlen = sizeof(peer);  // NOTE: important!!! Or recvfrom can't get peer addr.
        size = recvfrom(server_socket, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &caddrlen);
        if(size<0)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        printf("recvfrom client: %s\n", buf);

        // char clientInfo[NI_MAXHOST+NI_MAXSERV];

        // inetAddressStr((struct sockaddr *)&peer, sizeof(peer), clientInfo, sizeof(clientInfo));

        // printf("Client: %s connecting.\n", clientInfo);

        size = sendto(server_socket, buf, sizeof(buf), 0, (struct sockaddr *)&peer, caddrlen);
        if(size<0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    };
    
    printf("UDP server exit!!!\r\n");
    EXIT(0);
}