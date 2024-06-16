#include "etcp.h"

int main(int argc, char **argv)
{
    ssize_t size;
    char buf[100];

    if(argc != 3)
    {
        printf("Please input correct parameters!\n");
        return -1;
    }
    memcpy(buf, "hello", 6);

    int client_socket = tcp_client(argv[1], argv[2]);

    while(client_socket)
    {
        size = send(client_socket, buf, sizeof(buf), 0);
        if(size<0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }

        size = recv(client_socket, buf, sizeof(buf), 0);
        if(size<0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        printf("recv from server: %s\n", buf);
    };
    
    printf("TCP client exit!!!\r\n");
    EXIT(0);
}

