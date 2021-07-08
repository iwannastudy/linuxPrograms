#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    sigset_t blockSet, prevMask;

    /* Initialize a signal set to contain SIGINT */
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);

    // Block SIGINT, save previous signal mask
    if(sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Code that should not be interrupted by SIGINT

    // Restore previous signal mask, unblocking SIGINT
    if(sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Block all signals EXCEPT SIGKILL and SIGSTOP
    sigfillset(&blockSet);
    if(sigprocmask(SIG_BLOCK, &blockSet, NULL) == -1)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    return 0;
}
