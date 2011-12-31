
.PHONY : all install clean distclean

include config.mk


all: src/fizmo-ncursesw/fizmo-ncursesw

src/fizmo-ncursesw/fizmo-ncursesw::
	cd src/fizmo-ncursesw ; make

install: src/fizmo-ncursesw/fizmo-ncursesw
	mkdir -p $(bindir)
	cp src/fizmo-ncursesw/fizmo-ncursesw $(bindir)
	mkdir -p $(mandir)/man6
	cp src/man/fizmo-ncursesw.6 $(mandir)/man6
	mkdir -p $(localedir)
	for l in `cd src/locales ; ls -d ??_??`; \
	  do \
	  mkdir -p $(localedir)/$$l; \
	  cp src/locales/$$l/*.txt $(localedir)/$$l; \
	done

clean:
	cd src/fizmo-ncursesw ; make clean

distclean: clean
	cd src/fizmo-ncursesw ; make distclean

