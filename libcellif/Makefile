
.PHONY : all install install-dev install-locales clean distclean

include config.mk

ifeq ($(DEV_INSTALL_PREFIX),)
  DEV_INSTALL_PREFIX=$(INSTALL_PREFIX)
endif
PKG_DIR = $(DEV_INSTALL_PREFIX)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libcellif.pc


all: libcellif.a

libcellif.a: src/cell_interface/libcellif.a
	mv src/cell_interface/libcellif.a .

src/cell_interface/libcellif.a::
	cd src/cell_interface ; make

install:: install-locales

install-dev:: libcellif.a
	mkdir -p $(DEV_INSTALL_PREFIX)/lib/fizmo
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/cell_interface
	cp src/cell_interface/*.h \
	  $(DEV_INSTALL_PREFIX)/include/fizmo/cell_interface
	cp libcellif.a $(DEV_INSTALL_PREFIX)/lib/fizmo
	cp src/screen_interface/screen_cell_interface.h \
	  $(DEV_INSTALL_PREFIX)/include/fizmo/screen_interface
	mkdir -p $(PKG_DIR)
	echo 'prefix=$(DEV_INSTALL_PREFIX)' >$(PKGFILE)
	echo 'exec_prefix=$${prefix}' >>$(PKGFILE)
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>$(PKGFILE)
	echo 'includedir=$${prefix}/include/fizmo' >>$(PKGFILE)
	echo >>$(PKGFILE)
	echo 'Name: libcellif' >>$(PKGFILE)
	echo 'Description: libcellif' >>$(PKGFILE)
	echo 'Version: 0.7.0-b8' >>$(PKGFILE)
	echo 'Requires: libfizmo >= 0.7 ' >>$(PKGFILE)
	echo 'Requires.private:' >>$(PKGFILE)
	echo 'Cflags: -I$(DEV_INSTALL_PREFIX)/include/fizmo ' >>$(PKGFILE)
	echo 'Libs: -L$(DEV_INSTALL_PREFIX)/lib/fizmo -lcellif'  >>$(PKGFILE)
	echo >>$(PKGFILE)

install-locales::
	mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	  cp src/locales/$$l/*.txt $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	done

clean::
	cd src/cell_interface ; make clean
	cd src/locales ; make clean

distclean:: clean
	rm -f libcellif.a
	cd src/cell_interface ; make distclean

