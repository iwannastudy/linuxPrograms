# Makefile to build library used by all programs
#
# This make file relies on the assumption that each C file in this
# directory belongs in the library
#
# This makefile is very simple so that every version of make 
# should be able to handle it
#
include ../Makefile.inc

# The library build is "brute force" -- we don't bother with 
# dependency checking.

VPATH = inc
FLAG += -Iinc
#OBJ=error_functions.o get_num.o

all:${TLPI_LIB}

${TLPI_LIB} : src/*.c ename.c.inc
	$(CC) -c ${CFLAGS} ${FLAG}  src/*.c
	$(RM) ${TLPI_LIB}
	$(AR) rs ${TLPI_LIB} *.o

#%.o:%.c
#	$(CC) -c $^ -o $@ $(FLAG)

clean:
	$(RM) *.o ${TLPI_LIB}
