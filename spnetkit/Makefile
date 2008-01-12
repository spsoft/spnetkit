
#--------------------------------------------------------------------

CC = gcc
AR = ar cru
CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -g -fPIC
SOFLAGS = -shared -Wl,-h,$@
LDFLAGS = -lstdc++

LINKER = $(CC)
LINT = lint -c
RM = /bin/rm -f

ifeq ($(origin version), undefined)
	version = 0.1
endif

OS=$(shell uname)

ifeq ($(OS), SunOS)
	LDFLAGS += -lnsl -lsocket
	CFLAGS += -D_POSIX_THREAD_PROCESS_SHARED
endif

#--------------------------------------------------------------------

LIBOBJS = spnklog.o spnkbase64.o spnkini.o spnkfile.o spnkstr.o \
		spnksocket.o spnklist.o spnkreader.o \
		spnkpop3cli.o \
		spnksmtpaddr.o spnksmtpcli.o \
		spnkhttpmsg.o spnkhttpcli.o

SONAME = libspnetkit.so.$(version)

TARGET =  libspnetkit.so \
		testini \
		testpop3cli \
		testsmtpcli \
		testhttpcli

SONAME_S = libspnetkit_s.so.$(version)

TARGET_S = libspnetkit_s.so \
		testpop3cli_s \
		testsmtpcli_s

#--------------------------------------------------------------------

all: $(TARGET)

ssl: $(TARGET_S)

libspnetkit.so: $(SONAME)
	test -f $@ || ln -s $< $@

libspnetkit_s.so: $(SONAME_S)
	test -f $@ || ln -s $< $@

$(SONAME): $(LIBOBJS)
	$(LINKER) $(SOFLAGS) $^ -o $@

$(SONAME_S): spnksslsocket.o
	$(LINKER) $(SOFLAGS) $^ -o $@

testini: testini.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testpop3cli: testpop3cli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testsmtpcli: testsmtpcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testpop3cli_s: testpop3cli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L. -lspnetkit -lspnetkit_s -L/usr/lib -lssl -lcrypto

testsmtpcli_s: testsmtpcli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L. -lspnetkit -lspnetkit_s -L/usr/lib -lssl -lcrypto

testhttpcli: testhttpcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

dist: clean spnetkit-$(version).src.tar.gz

spnetkit-$(version).src.tar.gz:
	@ls | grep -v CVS | grep -v "\.so" | sed 's:^:spnetkit-$(version)/:' > MANIFEST
	@(cd ..; ln -s spnetkit spnetkit-$(version))
	(cd ..; tar cvf - `cat spnetkit/MANIFEST` | gzip > spnetkit/spnetkit-$(version).src.tar.gz)
	@(cd ..; rm spnetkit-$(version))

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) $(TARGET_S) $(SONAME) $(SONAME_S) )

#--------------------------------------------------------------------

# make rule
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@	

