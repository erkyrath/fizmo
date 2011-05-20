
.PHONY : all install install-dev install-locales clean distclean

include config.mk

TMPLIBDIR = libfizmotmp
ifeq ($(DEV_INSTALL_PREFIX),)
  DEV_INSTALL_PREFIX=$(INSTALL_PREFIX)
endif
PKG_DIR = $(DEV_INSTALL_PREFIX)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libfizmo.pc

all: libfizmo.a

libfizmo.a: src/tools/libtools.a src/interpreter/libinterpreter.a
	mkdir -p $(TMPLIBDIR) ; \
	cd $(TMPLIBDIR) ; \
	$(AR) x ../src/tools/libtools.a ; \
	$(AR) x ../src/interpreter/libinterpreter.a ; \
	$(AR) rcs ../libfizmo.a *.o ; \
	cd .. ; \
	rm -r $(TMPLIBDIR)

install:: install-locales

install-dev:: libfizmo.a
	mkdir -p $(DEV_INSTALL_PREFIX)/lib/fizmo
	cp libfizmo.a $(DEV_INSTALL_PREFIX)/lib/fizmo
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/interpreter
	cp src/interpreter/*.h $(DEV_INSTALL_PREFIX)/include/fizmo/interpreter
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/tools
	cp src/tools/*.h $(DEV_INSTALL_PREFIX)/include/fizmo/tools
	mkdir -p $(DEV_INSTALL_PREFIX)/include/fizmo/screen_interface
	cp src/screen_interface/*.h \
	  $(DEV_INSTALL_PREFIX)/include/fizmo/screen_interface
	cp src/screen_interface/*.cpp \
	  $(DEV_INSTALL_PREFIX)/include/fizmo/screen_interface
	cp -r src/sound_interface $(DEV_INSTALL_PREFIX)/include/fizmo/
	cp -r src/filesys_interface $(DEV_INSTALL_PREFIX)/include/fizmo/
	mkdir -p $(PKG_DIR)
	echo 'prefix=$(DEV_INSTALL_PREFIX)' >$(PKGFILE)
	echo 'exec_prefix=$${prefix}' >>$(PKGFILE)
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>$(PKGFILE)
	echo 'includedir=$${prefix}/include/fizmo' >>$(PKGFILE)
	echo >>$(PKGFILE)
	echo 'Name: libfizmo' >>$(PKGFILE)
	echo 'Description: libfizmo' >>$(PKGFILE)
	echo 'Version: 0.7.0-b8' >>$(PKGFILE)
ifeq ($(LIBFIZMO_REQS),)
	echo 'Requires:' >>$(PKGFILE)
else
	echo 'Requires: $(LIBFIZMO_REQS)' >>$(PKGFILE)
endif
	echo 'Requires.private:' >>$(PKGFILE)
	echo 'Cflags: -I$(DEV_INSTALL_PREFIX)/include/fizmo $(LIBXML2_NONPKG_CFLAGS)' >>$(PKGFILE)
	echo 'Libs: -L$(DEV_INSTALL_PREFIX)/lib/fizmo -lfizmo $(LIBXML2_NONPKG_LIBS)'  >>$(PKGFILE)
	echo >>$(PKGFILE)

install-locales::
	mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	  cp src/locales/$$l/*.txt $(INSTALL_PREFIX)/share/fizmo/locales/$$l; \
	done

clean::
	cd src/interpreter ; make clean
	cd src/tools ; make clean
	cd src/locales ; make clean

distclean:: clean
	rm -f libfizmo.a
	cd src/interpreter ; make distclean
	cd src/tools ; make distclean

src/tools/libtools.a::
	cd src/tools ; make

src/interpreter/libinterpreter.a::
	cd src/interpreter ; make

