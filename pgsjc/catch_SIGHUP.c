#include <unistd.h>
#include <signal.h>
#include "tlpi_hdr.h"
#define _XOPEN_SOURCE 500

static void handler(int sig)
{

}

int main(int argc, char **argv)
{
    pid_t childPid;
    struct sigaction sa;

    setbuf(stdout, NULL);       /* Make stdout unbuffered */

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;
    if( sigaction(SIGHUP, &sa, NULL) == -1 )
        errExit("sigaction");

    childPid = fork();
    if( childPid == -1 )
        errExit("fork");
    if( childPid == 0 && argc > 1 )
        if( setpgid(0, 0) == -1 )   // Move to new process group
            errExit("setpgig");
    printf("PID=%ld; PPID=%ld; PGID=%ld; SID=%ld\n", (long)getpid(),
            (long)getppid(), (long)getpgrp(), (long)getsid(0));
    alarm(60);                      // An unhandled SIGALRM ensures this process
                                    // will die if nothing else terminates it
    for(;;)
    {
        pause();
        printf("%ld: caught SIGHUP\n", (long)getpid());
    }
    return 0;
}

// Usage to test: when shell is signaled a SIGHUP, it will deliver SIGHUP to
// the processes which are created by itselt. And it will NOT deliver SIGHUP
// to the precesses which are NOT created by itselt.
// echo $$      -> echo current shell PID
// ./catch_SIGHUP > samegroup.log 2>&1 &
// ./catch_SIGHUP x > diffgroup.log 2>$1
// check the two output file and compare
