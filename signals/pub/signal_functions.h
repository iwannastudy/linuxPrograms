#ifndef SIGNAL_FUNCTIONS_H
#define SIGNAL_FUNCTIONS_H

#include <signal.h>
#include "tlpi_hdr.h"
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

int printSigMask(FILE *of, const char *msg);

int printPendingSigs(FILE *of, const char *msg);

void printSigset(FILE *of, const char *ldr, const sigset_t *mask);

#endif
