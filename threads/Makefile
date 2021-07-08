include ../Makefile.inc
#vpath check_password.c ./src
EXE = simple_thread

all : $(EXE)
simple_thread : simple_thread.o
	$(CC) -o $@ $^ ${LDLIBS} ${IMPL_THREAD_FLAGS}
%.o : %.c
	$(CC) -c -o $@ $^ ${CFLAGS}

clean:
	$(RM) *.o $(EXE)
