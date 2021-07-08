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
	int fd = -1;
	int ret;
	
	if(argc > 1)
	{
		ret = getopt(argc, argv, "a:i");
		switch( ret )
		{	
			case 'i':
				signal(SIGINT, SIG_IGN);

			case 'a':
				fd = open(argv[2], O_RDWR | O_APPEND);
				break;
		
			default:
				fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
		}
	}
	
	while( (n = read(STDIN_FILENO, buffer, MAX_READ)) > 0 )
	{
		//n = read(STDIN_FILENO, buffer, MAX_READ);
		//if( n == -1 )
		//{
		//	perror("read");
		//	exit(0);
		//}
		buffer[n] = '\0';
		printf("%s",buffer);

		if( fd > 0 )
		{
			n = write(fd, buffer, n);
			if( n == -1 )
			{
				perror("write");
				exit(0);
			}
		}

	}

	return 0;
}
