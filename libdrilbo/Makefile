
.PHONY : all install install-dev install-locales clean distclean

include config.mk

ifeq ($(fizmo_build_prefix),)
  fizmo_build_prefix="$(prefix)"
endif
PKG_DIR = $(fizmo_build_prefix)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libdrilbo.pc


all: libdrilbo.a

libdrilbo.a: src/drilbo/libdrilbo.a
	mv src/drilbo/libdrilbo.a .

test:
	cd src/drilbo ; make test

install:: install-locales

install-dev:: libdrilbo.a
	mkdir -p "$(fizmo_build_prefix)"/lib/fizmo
	cp libdrilbo.a "$(fizmo_build_prefix)"/lib/fizmo
	mkdir -p "$(fizmo_build_prefix)"/include/fizmo/drilbo
	cp src/drilbo/*.h "$(fizmo_build_prefix)"/include/fizmo/drilbo
	mkdir -p "$(PKG_DIR)"
	echo 'prefix=$(fizmo_build_prefix)' >"$(PKGFILE)"
	echo 'exec_prefix=$${prefix}' >>"$(PKGFILE)"
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>"$(PKGFILE)"
	echo 'includedir=$${prefix}/include/fizmo' >>"$(PKGFILE)"
	echo >>"$(PKGFILE)"
	echo 'Name: libdrilbo' >>"$(PKGFILE)"
	echo 'Description: libdrilbo' >>"$(PKGFILE)"
	echo 'Version: 0.2.1' >>"$(PKGFILE)"
ifeq ($(DRILBO_PKG_REQS),)
	echo 'Requires: libfizmo >= 0.7 ' >>"$(PKGFILE)"
else
	echo 'Requires: libfizmo >= 0.7, $(DRILBO_PKG_REQS) ' >>"$(PKGFILE)"
endif
	echo 'Requires.private:' >>"$(PKGFILE)"
	echo 'Cflags: -I$(fizmo_build_prefix)/include/fizmo $(DRILBO_NONPKG_X11_CFLAGS) $(DRILBO_NONPKG_LIBJPEG_CFLAGS) $(DRILBO_NONPKG_LIBPNG_CFLAGS) ' >>"$(PKGFILE)"
	echo 'Libs: -L$(fizmo_build_prefix)/lib/fizmo -ldrilbo -lpthread $(DRILBO_NONPKG_X11_LIBS) $(DRILBO_NONPKG_LIBJPEG_LIBS) $(DRILBO_NONPKG_LIBPNG_LIBS)'  >>"$(PKGFILE)"
	echo >>"$(PKGFILE)"

install-locales::
	mkdir -p "$(localedir)"
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p "$(localedir)/$$l" ; \
	  cp src/locales/$$l/*.txt "$(localedir)/$$l" ; \
	done

clean::
	cd src/drilbo ; make clean

distclean:: clean
	rm -f libdrilbo.a
	cd src/drilbo ; make distclean

src/drilbo/libdrilbo.a::
	cd src/drilbo ; make

