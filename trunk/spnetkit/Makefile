
include ../port/port.mk

#--------------------------------------------------------------------

ifeq ($(origin version), undefined)
	version = 0.3.0
endif

LIBOBJS = spnklog.o spnkbase64.o spnkini.o spnkfile.o spnkstr.o \
		spnktime.o spnkhash.o \
		spnksocket.o spnklist.o spnkreader.o \
		spnkendpoint.o spnksocketpool.o \
		spnkpop3cli.o \
		spnksmtpaddr.o spnksmtpcli.o \
		spnkhttpmsg.o spnkhttpcli.o \
		spnkmemobj.o spnkmemcli.o \
		spnkicapcli.o spnkmiltercli.o

SONAME = libspnetkit.so.$(version)

TARGET =  libspnetkit.so libspnetkit.a \
		testini \
		testpop3cli \
		testsmtpcli \
		testhttpcli \
		testmemproto \
		testmemcli \
		testendpoint \
		testsocketpool \
		testicapcli \
		testmiltercli

SONAME_S = libspnetkit_s.so.$(version)

TARGET_S = libspnetkit_s.so \
		testpop3cli_s \
		testsmtpcli_s

OPENSSL_INCL = -I/usr/include/openssl/
OPENSSL_LIB  = -L/usr/lib -lssl -lcrypto

CFLAGS += $(OPENSSL_INCL)

#--------------------------------------------------------------------

all: $(TARGET)

ssl: $(TARGET_S)

libspnetkit.so: $(SONAME)
	cd ../lib; test -f $@ || ln -s $< $@; exit 0;

libspnetkit_s.so: $(SONAME_S)
	cd ../lib; test -f $@ || ln -s $< $@; exit 0;

$(SONAME): $(LIBOBJS)
	$(LINKER) $(SOFLAGS) $^ -o $@
	$(INSTLIB)

libspnetkit.a: $(LIBOBJS)
	$(AR) $@ $^

$(SONAME_S): spnksslsocket.o
	$(LINKER) $(SOFLAGS) $^ -o $@
	$(INSTLIB)

testini: testini.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testpop3cli: testpop3cli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testsmtpcli: testsmtpcli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testpop3cli_s: testpop3cli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L../lib -lspnetkit -lspnetkit_s $(OPENSSL_LIB)

testsmtpcli_s: testsmtpcli_s.o
	$(LINKER) $(LDFLAGS) -o $@ $^ -L../lib -lspnetkit -lspnetkit_s $(OPENSSL_LIB)

testhttpcli: testhttpcli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testmemproto: testmemproto.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testmemcli: testmemcli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testendpoint: testendpoint.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testsocketpool: testsocketpool.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testicapcli: testicapcli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

testmiltercli: testmiltercli.o
	$(LINKER) $(LDFLAGS) $^ -L../lib -lspnetkit -o $@

dist: clean spnetkit-$(version).src.tar.gz

spnetkit-$(version).src.tar.gz:
	@ls | grep -v CVS | grep -v "\.so" | sed 's:^:spnetkit-$(version)/:' > MANIFEST
	@(cd ..; ln -s spnetkit spnetkit-$(version))
	(cd ..; tar cvf - `cat spnetkit/MANIFEST` | gzip > spnetkit/spnetkit-$(version).src.tar.gz)
	@(cd ..; rm spnetkit-$(version))

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) $(TARGET_S) $(SONAME) $(SONAME_S) )

