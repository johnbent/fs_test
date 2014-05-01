# LIBRARY_BUILD
INSTALL_ROOT = /scratch/iod
CFLAGS += -g -I${INSTALL_ROOT}/include -O0
#CFLAGS += -g -Wall -Werror -I${INSTALL_ROOT}/include 
LDLIBS += -L${INSTALL_ROOT}/lib -liod -lplfs -lpthread -ldaos
CPPFLAGS = ${CFLAGS}

C_SRCS = $(wildcard *.c) 
C_TARGETS = $(basename $(C_SRCS) )
CPP_SRCS = $(wildcard *.C) 
CPP_TARGETS = $(basename $(CPP_SRCS) )

CC = mpicc
CXX = mpicxx

all: $(C_TARGETS) $(CPP_TARGETS)

run_test:
	LD_LIBRARY_PATH=${INSTALL_ROOT}/lib/ ./simple_example

test:
	./array_test

clean:
	rm -f *.o $(C_TARGETS) $(CPP_TARGETS)

cleanup:
	/scratch/iod/scripts/purgeall.sh	
