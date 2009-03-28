
include port/port.mk

#--------------------------------------------------------------------

ifeq ($(origin version), undefined)
	version = 0.3.0
endif

all:
	mkdir -p lib
	@( cd spnetkit; make )

ssl:
	@( cd spnetkit;  make ssl )

dist: clean spnetkit-$(version).src.tar.gz

spnetkit-$(version).src.tar.gz:
	@find . -type f | grep -v CVS | grep -v .svn | sed s:^./:spnetkit-$(version)/: > MANIFEST
	@(cd ..; ln -s spnetkit spnetkit-$(version))
	(cd ..; tar cvf - `cat spnetkit/MANIFEST` | gzip > spnetkit/spnetkit-$(version).src.tar.gz)
	@(cd ..; rm spnetkit-$(version))

clean:
	@( rm -rf lib/*; cd spnetkit; make clean )

