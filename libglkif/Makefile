
.PHONY : all install install-dev install-locales clean distclean

include config.mk

ifeq ($(DEV_INSTALL_PREFIX),)
  DEV_INSTALL_PREFIX=$(INSTALL_PREFIX)
endif
PKG_DIR = $(DEV_INSTALL_PREFIX)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libglkif.pc


all: libglkif.a

libglkif.a: src/glk_interface/libglkif.a
	mv src/glk_interface/libglkif.a .

src/glk_interface/libglkif.a::
	cd src/glk_interface ; make

install:: install-locales

install-dev:: libglkif.a
	mkdir -p $(DEV_INSTALL_PREFIX)/lib/fizmo
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/glk_interface
	cp src/glk_interface/*.h \
	  $(DEV_INSTALL_PREFIX)/include/fizmo/glk_interface
	cp libglkif.a $(DEV_INSTALL_PREFIX)/lib/fizmo
	mkdir -p $(PKG_DIR)
	echo 'prefix=$(DEV_INSTALL_PREFIX)' >$(PKGFILE)
	echo 'exec_prefix=$${prefix}' >>$(PKGFILE)
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>$(PKGFILE)
	echo 'includedir=$${prefix}/include/fizmo' >>$(PKGFILE)
	echo >>$(PKGFILE)
	echo 'Name: libglkif' >>$(PKGFILE)
	echo 'Description: libglkif' >>$(PKGFILE)
	echo 'Version: 0.1.0' >>$(PKGFILE)
	echo 'Requires: libfizmo >= 0.7 ' >>$(PKGFILE)
	echo 'Requires.private:' >>$(PKGFILE)
	echo 'Cflags: -I$(DEV_INSTALL_PREFIX)/include/fizmo ' >>$(PKGFILE)
	echo 'Libs: -L$(DEV_INSTALL_PREFIX)/lib/fizmo -lfizmo'  >>$(PKGFILE)
	echo >>$(PKGFILE)

install-locales::
	mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	  cp src/locales/$$l/*.txt $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	done

clean::
	cd src/glk_interface ; make clean
	cd src/locales ; make clean

distclean:: clean
	rm -f libglkif.a
	cd src/glk_interface ; make distclean

