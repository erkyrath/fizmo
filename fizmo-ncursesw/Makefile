
.PHONY : all install clean distclean

include config.mk


all: src/fizmo-ncursesw/fizmo-ncursesw

src/fizmo-ncursesw/fizmo-ncursesw::
	cd src/fizmo-ncursesw ; make

install: src/fizmo-ncursesw/fizmo-ncursesw
	mkdir -p $(DESTDIR)$(bindir)
	cp src/fizmo-ncursesw/fizmo-ncursesw $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(mandir)/man6
	cp src/man/fizmo-ncursesw.6 $(DESTDIR)$(mandir)/man6
	mkdir -p $(DESTDIR)$(localedir)
	for l in `cd src/locales ; ls -d ??_??`; \
	  do \
	  mkdir -p $(DESTDIR)$(localedir)/$$l; \
	  cp src/locales/$$l/*.txt $(DESTDIR)$(localedir)/$$l; \
	done

clean:
	cd src/fizmo-ncursesw ; make clean

distclean: clean
	cd src/fizmo-ncursesw ; make distclean

