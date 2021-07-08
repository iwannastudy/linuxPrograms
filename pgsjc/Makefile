include ../Makefile.inc
#vpath check_password.c ./src
EXE = catch_SIGHUP

all : $(EXE)
catth_SIGHUB : catch_SIGHUP.o
	$(CC) -o $@ $^ ${LDLIBS}
%.o : %.c
	$(CC) -c -o $@ $^ ${CFLAGS}

clean:
	$(RM) *.o $(EXE)
