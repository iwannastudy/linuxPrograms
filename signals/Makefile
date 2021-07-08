include ../Makefile.inc
#vpath check_password.c ./src
EXE = signal sigHandler kill

all : $(EXE)
signal : signal.o
	$(CC) -o $@ $^ ${LDLIBS}
sigHandler : sigHandler.o
	$(CC) -o $@ $^ ${LDLIBS}
kill : kill.o
	$(CC) -o $@ $^ ${LDLIBS}
%.o : %.c
	$(CC) -c -o $@ $^ ${CFLAGS}

clean:
	$(RM) *.o $(EXE)
