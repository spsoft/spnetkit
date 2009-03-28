
#--------------------------------------------------------------------

CC = gcc
AR = ar cru
CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -g -fPIC
SOFLAGS = -shared -Wl,-h,$@
LDFLAGS = -lstdc++ -lpthread

LINKER = $(CC)
LINT = lint -c
RM = /bin/rm -f

OS=$(shell uname)

ifeq ($(OS), SunOS)
	LDFLAGS += -lnsl -lsocket
	CFLAGS += -D_POSIX_THREAD_PROCESS_SHARED
endif

INSTLIB=(mkdir -p ../lib; cp $@ ../lib)

#--------------------------------------------------------------------

# make rule
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@	

