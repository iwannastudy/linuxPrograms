include ../Makefile.inc
vpath check_password.c ./src
EXE = check_password

all : $(EXE)
check_password : check_password.o
	$(CC) -o $@ $^ ${LDLIBS} ${LINUX_LIBCRYPT}
check_password.o : check_password.c
	$(CC) -c -o $@ $^ ${CFLAGS}

clean:
	$(RM) *.o $(EXE)
