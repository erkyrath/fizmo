
.PHONY : all install clean distclean

include config.mk

ifneq ($(FIZMO_BIN_DIR),)
  INSTALL_BIN_DIR = $(FIZMO_BIN_DIR)
else
  INSTALL_BIN_DIR = games
endif


all: src/fizmo-ncursesw/fizmo-ncursesw

src/fizmo-ncursesw/fizmo-ncursesw::
	cd src/fizmo-ncursesw ; make

install: src/fizmo-ncursesw/fizmo-ncursesw
	mkdir -p $(INSTALL_PREFIX)/$(INSTALL_BIN_DIR)
	cp src/fizmo-ncursesw/fizmo-ncursesw $(INSTALL_PREFIX)/$(INSTALL_BIN_DIR)
	mkdir -p $(INSTALL_PREFIX)/man/man6
	cp src/man/fizmo-ncursesw.6 $(INSTALL_PREFIX)/man/man6
	mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales
	for l in `cd src/locales ; ls -d ??_??`; \
	  do \
	  mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	  cp src/locales/$$l/*.txt $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	done

clean:
	cd src/fizmo-ncursesw ; make clean

distclean: clean
	cd src/fizmo-ncursesw ; make distclean

