include ../Makefile.inc
VPATH=src:test
SUBDIR=../lib
vpath get_num.o $(SUBDIR)
PUB_PATH=-I$(SUBDIR)/pub
INC_PATH=-Isrc

Target=test_become_daemon

OBJ=becomeDaemon.o test_become_daemon.o get_num.o

subsystem:
	$(MAKE) -C $(SUBDIR)
	$(MAKE) all

all:$(Target)
$(Target):$(OBJ)
	$(CC) $^ -o $@
#$(OBJ).o:$(OBJ).c
%.o:%.c
	$(CC) -c $< -o $@ $(PUB_PATH) $(INC_PATH) $(CFLAGS)
#$(Target).o:$(Target).c 
#	$(CC) -c $^ -o $@ $(FLAG)

.PHONY: clean
clean:
	rm -f *.o 
	rm -f $(Target)
	$(MAKE) -w clean -C $(SUBDIR)
