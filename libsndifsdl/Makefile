
.PHONY : all install install-dev install-locales clean distclean

include config.mk

ifeq ($(fizmo_build_prefix),)
  fizmo_build_prefix="$(DESTDIR)$(prefix)"
endif
PKG_DIR = $(fizmo_build_prefix)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libsndifsdl.pc


all: libsndifsdl.a

libsndifsdl.a: src/sound_sdl/libsndifsdl.a
	mv src/sound_sdl/libsndifsdl.a .

src/sound_sdl/libsndifsdl.a::
	cd src/sound_sdl; make

install:: install-locales

install-dev:: libsndifsdl.a
	mkdir -p $(fizmo_build_prefix)/lib/fizmo
	cp libsndifsdl.a $(fizmo_build_prefix)/lib/fizmo
	mkdir -p $(fizmo_build_prefix)/include/fizmo/sound_sdl
	cp src/sound_sdl/sound_sdl.h \
	  $(fizmo_build_prefix)/include/fizmo/sound_sdl
	mkdir -p $(PKG_DIR)
	echo 'prefix=$(fizmo_build_prefix)' >$(PKGFILE)
	echo 'exec_prefix=$${prefix}' >>$(PKGFILE)
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>$(PKGFILE)
	echo 'includedir=$${prefix}/include/fizmo' >>$(PKGFILE)
	echo >>$(PKGFILE)
	echo 'Name: libsndifsdl' >>$(PKGFILE)
	echo 'Description: libsndifsdl' >>$(PKGFILE)
	echo 'Version: 0.7.2' >>$(PKGFILE)
ifeq ($(SOUNDSDL_PKG_REQS),)
	echo 'Requires: libfizmo >= 0.7' >>$(PKGFILE)
else
	echo 'Requires: libfizmo >= 0.7, $(SOUNDSDL_PKG_REQS)' >>$(PKGFILE)
endif
	echo 'Requires.private:' >>$(PKGFILE)
	echo 'Cflags: -I$(fizmo_build_prefix)/include/fizmo $(SOUNDSDL_NONPKG_CFLAGS)' >>$(PKGFILE)
	echo 'Libs: -L$(fizmo_build_prefix)/lib/fizmo -lsndifsdl $(SOUNDSDL_PKG_LIBS)'  >>$(PKGFILE)
	echo >>$(PKGFILE)

install-locales:


clean::
	cd src/sound_sdl ; make clean

distclean:: clean
	rm -f libsndifsdl.a
	cd src/sound_sdl; make distclean

