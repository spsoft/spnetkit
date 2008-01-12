
include port/port.mk

#--------------------------------------------------------------------

all:
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
	@( cd spnetkit; make clean )

