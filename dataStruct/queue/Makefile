VPATH=src:test
test=testQueueArr
obj=queueArray.o
flag= -Isrc

all:$(obj)
%.o:%.c
	gcc -c $^ -o $@

test:$(test)
$(test):$(test).o $(obj)
	gcc $^ -o $@
$(test).o:$(test).c
	gcc -c $^ -o $@ $(flag)


.PHONY: clean
clean:
	rm -f *.o $(test)
