
include ../port/port.mk

#--------------------------------------------------------------------

ifeq ($(origin version), undefined)
	version = 0.3.0
endif

LIBOBJS = spnklog.o spnkbase64.o spnkini.o spnkfile.o spnkstr.o \
		spnktime.o spnkhash.o spnkbuffer.o \
		spnksocket.o spnklist.o spnkreader.o \
		spnkendpoint.o spnksocketpool.o \
		spnkpop3cli.o \
		spnksmtpaddr.o spnksmtpcli.o \
		spnkhttpmsg.o spnkhttputils.o spnkhttpcli.o \
		spnkmemobj.o spnkmemcli.o \
		spnkicapcli.o spnkmiltercli.o \
		spnklock.o spnkconfig.o spnkmd5.o \
		spnkserver.o spnkhttpsvr.o

TARGET =  libspnetkit.so libspnetkit.a \
		teststr \
		testini \
		testpop3cli \
		testsmtpcli \
		testhttpcli \
		testmemproto \
		testmemcli \
		testendpoint \
		testsocketpool \
		testicapcli \
		testmiltercli \
		testlock \
		testconfig \
		testserver \
		testhttpsvr

TARGET_S = libspnetkit_s.so libspnetkit_s.a \
		testpop3cli_s \
		testsmtpcli_s

OPENSSL_INCL = -I/usr/local/ssl/include/
OPENSSL_LIB  = -L/usr/lib -lssl -lcrypto

CFLAGS += $(OPENSSL_INCL)

#--------------------------------------------------------------------

all: $(TARGET)

ssl: $(TARGET_S)

libspnetkit.so: $(LIBOBJS)
	$(LINKER) $(SOFLAGS) $^ -o $@

libspnetkit.a: $(LIBOBJS)
	$(AR) $@ $^

libspnetkit_s.so: spnksslsocket.o spnksslsmtpcli.o
	$(LINKER) $(SOFLAGS) $^ -o $@

libspnetkit_s.a: spnksslsocket.o spnksslsmtpcli.o
	$(AR) $@ $^

testini: testini.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

teststr: teststr.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testpop3cli: testpop3cli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testsmtpcli: testsmtpcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testpop3cli_s: testpop3cli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L. -lspnetkit -lspnetkit_s $(OPENSSL_LIB)

testsmtpcli_s: testsmtpcli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L. -lspnetkit -lspnetkit_s $(OPENSSL_LIB)

testhttpcli: testhttpcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testmemproto: testmemproto.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testmemcli: testmemcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testendpoint: testendpoint.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testsocketpool: testsocketpool.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testicapcli: testicapcli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testmiltercli: testmiltercli.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testlock: testlock.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testconfig: testconfig.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testserver: testserver.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

testhttpsvr: testhttpsvr.o
	$(LINKER) $(LDFLAGS) $^ -L. -lspnetkit -o $@

dist: clean spnetkit-$(version).src.tar.gz

spnetkit-$(version).src.tar.gz:
	@ls | grep -v CVS | grep -v "\.so" | sed 's:^:spnetkit-$(version)/:' > MANIFEST
	@(cd ..; ln -s spnetkit spnetkit-$(version))
	(cd ..; tar cvf - `cat spnetkit/MANIFEST` | gzip > spnetkit/spnetkit-$(version).src.tar.gz)
	@(cd ..; rm spnetkit-$(version))

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) $(TARGET_S) $(SONAME) $(SONAME_S) )

