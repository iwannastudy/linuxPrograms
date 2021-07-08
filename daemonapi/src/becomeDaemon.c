#include <sys/stat.h>
#include <fcntl.h>
#include "becomeDaemon.h"
#include <unistd.h>
#include "tlpi_hdr.h"

/* Returns 0 on success, -1 on error
 */
int becomeDaemon(int flags)
{
    int maxfd, fd;

    switch(fork())                      // Become background process
    {
        case -1: return -1;
        case  0: break;                 // Child falls through...
        default: _exit(EXIT_SUCCESS);   // while parent terminates
    }

    if(setsid() == -1)                  // Become leader of new session
        return -1;
    
    switch(fork())                  // Ensure we are not session lader
    {
        case -1: return -1;
        case  0: break;
        default: _exit(EXIT_SUCCESS);
    }
    if(!(flags & BD_NO_UMASK0))
        umask(0);                       // clear file mode creation mask
    
    if(!(flags & BD_NO_CHDIR))
        chdir("/");                     // change to root directory

    if(!(flags & BD_NO_CLOSE_FILES))       // close all open files
    {
        maxfd = sysconf(_SC_OPEN_MAX);
        if(maxfd == -1)                     // limit is indeterminate...
            maxfd = BD_MAX_CLOSE;           // so take a guess

        for(fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if(!(flags & BD_NO_REOPEN_STD_FDS))
    {
        close(STDIN_FILENO);                // reopen standard fd's to /dev/null 

        fd = open("/dev/null", O_RDWR);

        if(fd != STDIN_FILENO)              // 'fd' should be 0
            return -1;
        if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}
