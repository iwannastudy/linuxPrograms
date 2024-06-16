#include "etcp.h"



int main(int argc, char **argv)
{
    struct sockaddr_in peer;
    socklen_t caddrlen = 0;
    ssize_t size;
    char buf[100];

    if(argc != 3)
    {
        printf("Please input correct parameters!\n");
        return -1;
    }
    memcpy(buf, "hello", 6);

    int client_socket = udp_client(argv[1], argv[2], &peer);
    caddrlen = sizeof(peer);

    while(client_socket)
    {
        size = sendto(client_socket, buf, sizeof(buf), 0, (struct sockaddr *)&peer, caddrlen);
        if(size<0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }

        size = recvfrom(client_socket, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &caddrlen);
        if(size<0)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        printf("recvfrom client: %s\n", buf);
    };
    
    printf("UDP client exit!!!\r\n");
    EXIT(0);
}
