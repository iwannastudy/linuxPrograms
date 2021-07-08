#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <termios.h>
//#include <errno.h>
//#include <limits.h>
#include "uart.h"
#define DEV_NAME "/dev/ttyS2"
int main(void)
{
    int iFd, i;
    int len;
    unsigned char ucBuf[1000];
 
    iFd = open_uart(DEV_NAME);
    //iFd = open(DEV_NAME, O_RDWR|O_NOCTTY);
    printf("iFd = %d\n", iFd);
    set_uart_attr(iFd, B115200, 8, 1, 'N', 0);

    for (i = 0; i < 1000; i++)
    {
        ucBuf[i] = i;
    }
    len = write(iFd, ucBuf, 0xff);
    if( len < 0 )
    {
        perror("write");
        close_uart(iFd);       
        return -1;
    }
    else
    {
        printf("write %d bytes success\n", len);
    }

    len = 0;
    int count = 0;
    while(count < 0xff)
    {
        printf("read data...\n");
        len = read(iFd, ucBuf + count, 0xff - count);
        if ( len < 0 )
        {
            perror("read");
            close(iFd);
            return -1;
        }
        else
        {
            printf("read date length = %d \n", len);
        }
        count = count + len;
    }
    for (i = 0; i < len; i++)
    {
        printf(" %x", ucBuf[i]);
    }
    printf("\nend\n"); 
    close_uart(iFd); 
    return 0;
}
