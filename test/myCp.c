#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_READ 1024

int main(int argc, char **argv)
{
	ssize_t n;
	char buffer[MAX_READ];
	int fdInput = -1, fdOutput = -1;
	int ret;
	
	if(argc < 3)
	{
		printf("the arguement is not enough!\n");
		return -1;
	}

	fdInput = open(argv[1], O_RDONLY);
	if( fdInput == -1 )
	{
		perror("read");
		return -1; 
	}

	fdOutput = open(argv[2], O_RDWR | O_CREAT | O_APPEND, 0644);
	
	while( (n = read(argv[1], buffer, MAX_READ)) > 0 )
	{
		if( fd > 0 )
		{
			n = write(argv[2], buffer, n);
			if( n == -1 )
			{
				perror("write");
				exit(0);
			}
		}

	}

	return 0;
}
