#ifndef __UART_H__
#define __UART_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

/***************************************************
 * @brief           open uart device file
 * @devName         uart device file
 * @return value    >0, success; -1, fail
 * *************************************************/
int open_uart(char *devName);

/***************************************************
 * @brief           close uart device
 * @uartFd          uart device fd
 * @return value    >0, success; -1, fail
 * *************************************************/
void close_uart(int uartFd);
 
/***************************************************
 * @brief           set uart parity
 * @dataBits        data bit, 7 or 8
 * @stopBits        stop bit, 1 or 2
 * @parity          parity, N, E, O, S
 * return value     0, success; -1, fail
 ***************************************************/
int set_uart_attr(int uartFd, int bandRate, int dataBits,
                  int stopBits, int parity, int flowCtrl);

#endif
