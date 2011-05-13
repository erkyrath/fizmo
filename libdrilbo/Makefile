
.PHONY : all install install-dev install-locales clean distclean

include config.mk

ifeq ($(DEV_INSTALL_PREFIX),)
  DEV_INSTALL_PREFIX=$(INSTALL_PREFIX)
endif
PKG_DIR = $(DEV_INSTALL_PREFIX)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libdrilbo.pc


all: libdrilbo.a

libdrilbo.a: src/drilbo/libdrilbo.a
	mv src/drilbo/libdrilbo.a .

install:: install-locales

install-dev:: libdrilbo.a
	mkdir -p $(DEV_INSTALL_PREFIX)/lib/fizmo
	cp libdrilbo.a $(DEV_INSTALL_PREFIX)/lib/fizmo
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/drilbo
	cp src/drilbo/*.h $(DEV_INSTALL_PREFIX)/include/fizmo/drilbo
	mkdir -p $(PKG_DIR)
	echo 'prefix=$(DEV_INSTALL_PREFIX)' >$(PKGFILE)
	echo 'exec_prefix=$${prefix}' >>$(PKGFILE)
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>$(PKGFILE)
	echo 'includedir=$${prefix}/include/fizmo' >>$(PKGFILE)
	echo >>$(PKGFILE)
	echo 'Name: libdrilbo' >>$(PKGFILE)
	echo 'Description: libdrilbo' >>$(PKGFILE)
	echo 'Version: 0.2.0-b2' >>$(PKGFILE)
ifeq ($(DRILBO_PKG_REQS),)
	echo 'Requires: libfizmo >= 0.7 ' >>$(PKGFILE)
else
	echo 'Requires: libfizmo >= 0.7, $(DRILBO_PKG_REQS) ' >>$(PKGFILE)
endif
	echo 'Requires.private:' >>$(PKGFILE)
	echo 'Cflags: -I$(DEV_INSTALL_PREFIX)/include/fizmo $(DRILBO_NONPKG_X11_CFLAGS) $(DRILBO_NONPKG_LIBJPEG_CFLAGS) $(DRILBO_NONPKG_LIBPNG_CFLAGS) ' >>$(PKGFILE)
	echo 'Libs: -L$(DEV_INSTALL_PREFIX)/lib/fizmo -ldrilbo -lpthread $(DRILBO_NONPKG_X11_LIBS) $(DRILBO_NONPKG_LIBJPEG_LIBS) $(DRILBO_NONPKG_LIBPNG_LIBS)'  >>$(PKGFILE)
	echo >>$(PKGFILE)

install-locales::
	mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	  cp src/locales/$$l/*.txt $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	done

clean::
	cd src/drilbo ; make clean

distclean:: clean
	rm -f libdrilbo.a
	cd src/drilbo ; make distclean

src/drilbo/libdrilbo.a::
	cd src/drilbo ; make

