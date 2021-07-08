#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    int fd = 0;
    fd = open("/tmp/verifyvfork.txt", O_CREAT|O_RDWR|)
}
