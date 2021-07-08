#include "uart.h"

int open_uart(char *devName)
{
    int uartFd;

    uartFd = open(devName, O_RDWR|O_NOCTTY|O_SYNC);
    if(uartFd < 0)
    {
        printf("fail to open dev file\n");
        return -1;
    }
    else
        printf("uart device fd = %d\n", uartFd);
    
    // // Testore serial device to block
    // if( fcntl(uartFd, F_SETFL, 0) < 0 )
    // {
    //     printf("fcntl failed!\n");
    //     return -1;
    // }
    // else
    // {
    //     //printf("");
    // }
    // // Test if is a tty device
    // if( 0 == isatty(STDIN_FILENO) )
    // {
    //     printf("standard input is not a terminal device\n");
    //     return -1;
    // }
    // else
    // {
    //     printf("is a tty\n");
    // }

    return uartFd;
}

void close_uart(int uartFd)
{
    close(uartFd);
}

int set_uart_attr(int uartFd, int bandRate, int dataBits, 
                  int stopBits, int parity, int flowCtrl)
{
    struct termios opt;

    if( tcgetattr(uartFd, &opt) != 0)
    {
        printf("get attr failed\n");
        return -1;
    }

    //tcflush(uartFd, TCIOFLUSH);
    cfsetispeed(&opt, bandRate);
    cfsetospeed(&opt, bandRate);

    // 修改控制模式，忽略调制解调器状态行
    opt.c_cflag |= CLOCAL;
    // 修改控制模式，使得能够从串口中读取输数据
    opt.c_cflag |= CREAD;
    // 修改输出模式，使不经处理、过滤的原始数据输出到串行设备接口 
    opt.c_oflag &= ~(OPOST);
    // 修改本地标志，原始模式，接收的数据不经处理
    opt.c_lflag &= ~(ECHO | ICANON | ECHOE | ISIG);
    //tcflush(uartFd, TCIOFLUSH);

    switch(flowCtrl)        // set flow ctrl
    {
        case 0:             // no hardware flow ctrl
            opt.c_iflag &= ~CRTSCTS;
            break;
        case 1:             // hardware flow ctrl
            opt.c_iflag |= CRTSCTS;
            break;
        case 2:             // software flow ctrl
            opt.c_iflag |= IXON | IXOFF | IXANY;
            break;
    }

    opt.c_cflag &= ~CSIZE;  // block other flags

    switch(dataBits)        // set data bits
    {
        case 5:
            opt.c_cflag |= CS5; break;
        case 6:
            opt.c_cflag |= CS6; break;
        case 7:
            opt.c_cflag |= CS7; break;
        case 8:
            opt.c_cflag |= CS8; break;
        default:
            printf("unsupported data size\n");
            return -1;
    }

    switch(parity)
    {
        case 'n':
        case 'N':                       // no parity
            opt.c_cflag &= ~PARENB;     // Disable parity
            opt.c_iflag &= ~INPCK;      // Disable parity checking
            break;
        case 'o':
        case 'O':                           // Odd
            opt.c_cflag |= (PARODD|PARENB); // 设置为奇校验
            opt.c_iflag |= INPCK;           // Enable parity checking
            break;
        case 'e':
        case 'E':                       // Even
            opt.c_cflag |= PARENB;      // Enable parity
            opt.c_cflag &= ~PARODD;     // 转换为偶校验
            opt.c_iflag |= INPCK;       // Enable parity checking
            break;
        case 's':
        case 'S':                       // Space
            opt.c_cflag &= ~PARENB;
            //opt.c_cflag &= ~CSTOPB;
            break;
        default:
            printf("unsupported parity\n");
            return -1;
    }

    switch(stopBits)        // set stop bits
    {
        case 1:
            opt.c_cflag &= ~CSTOPB; 
            break;
        case 2:
            opt.c_cflag |= CSTOPB;  
            break;
        default:
            printf("unsupported stop bits\n");
            return -1;
    }

    opt.c_cc[VTIME] = 50;    // Set timeout (n/10)s; 0->wait always
    opt.c_cc[VMIN]  = 255;  // 0->every 1 byte return; n->n bytes return

    // 如果发生数据溢出，接收数据，但是不在读取 刷新收到的数据但是不读
    //tcflush(uartFd, TCIFLUSH);

    if( tcsetattr(uartFd, TCSANOW, &opt) != 0 )
    {
        printf("failed to set attr");
        return -1;
    }
    return 0;
}
