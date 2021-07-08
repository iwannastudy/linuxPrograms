/*
	note:the read fun just read a character!!!
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFIZE 4096

int main()
{
	int n;
	char buf[BUFFIZE];

	while( (n = read(STDIN_FILENO, buf, BUFFIZE) > 0))
	{
		//printf("read n = %d\n",n);
		if( write(STDOUT_FILENO, buf, n) != n )
			perror("write error");
		//printf("write n = %d\n",n);
	}
	
	if( n < 0 )
		perror("readverror");
	
	exit( 0 );
}

