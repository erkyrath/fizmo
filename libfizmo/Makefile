
.PHONY : all install install-dev install-locales clean distclean

include config.mk

TMPLIBDIR = libfizmotmp
ifeq ($(fizmo_build_prefix),)
  fizmo_build_prefix="$(prefix)"
endif
PKG_DIR = $(fizmo_build_prefix)/lib/pkgconfig
PKGFILE = $(PKG_DIR)/libfizmo.pc


all: libfizmo.a

libfizmo.a: src/tools/libtools.a src/interpreter/libinterpreter.a
	mkdir -p "$(TMPLIBDIR)" ; \
	cd "$(TMPLIBDIR)" ; \
	"$(AR)" x ../src/tools/libtools.a ; \
	"$(AR)" x ../src/interpreter/libinterpreter.a ; \
	"$(AR)" rcs ../libfizmo.a *.o ; \
	cd .. ; \
	rm -r "$(TMPLIBDIR)"

install:: install-locales

install-dev:: libfizmo.a
	mkdir -p "$(fizmo_build_prefix)"/lib/fizmo
	cp libfizmo.a "$(fizmo_build_prefix)"/lib/fizmo
	mkdir -p "$(fizmo_build_prefix)"/include/fizmo/interpreter
	cp src/interpreter/*.h "$(fizmo_build_prefix)"/include/fizmo/interpreter
	mkdir -p "$(fizmo_build_prefix)"/include/fizmo/tools
	cp src/tools/*.h "$(fizmo_build_prefix)"/include/fizmo/tools
	mkdir -p "$(fizmo_build_prefix)"/include/fizmo/screen_interface
	cp src/screen_interface/*.h \
	  "$(fizmo_build_prefix)"/include/fizmo/screen_interface
	cp src/screen_interface/*.cpp \
	  "$(fizmo_build_prefix)"/include/fizmo/screen_interface
	cp -r src/sound_interface "$(fizmo_build_prefix)"/include/fizmo/
	cp -r src/filesys_interface "$(fizmo_build_prefix)"/include/fizmo/
	cp -r src/blorb_interface "$(fizmo_build_prefix)"/include/fizmo/
	mkdir -p "$(PKG_DIR)"
	echo 'prefix=$(fizmo_build_prefix)' >"$(PKGFILE)"
	echo 'exec_prefix=$${prefix}' >>"$(PKGFILE)"
	echo 'libdir=$${exec_prefix}/lib/fizmo' >>"$(PKGFILE)"
	echo 'includedir=$${prefix}/include/fizmo' >>"$(PKGFILE)"
	echo >>"$(PKGFILE)"
	echo 'Name: libfizmo' >>"$(PKGFILE)"
	echo 'Description: libfizmo' >>"$(PKGFILE)"
	echo 'Version: 0.7.1' >>"$(PKGFILE)"
ifeq ($(LIBFIZMO_REQS),)
	echo 'Requires:' >>"$(PKGFILE)"
else
	echo 'Requires: $(LIBFIZMO_REQS)' >>"$(PKGFILE)"
endif
	echo 'Requires.private:' >>"$(PKGFILE)"
	echo 'Cflags: -I$(fizmo_build_prefix)/include/fizmo $(LIBXML2_NONPKG_CFLAGS)' >>"$(PKGFILE)"
	echo 'Libs: -L$(fizmo_build_prefix)/lib/fizmo -lfizmo $(LIBXML2_NONPKG_LIBS)'  >>"$(PKGFILE)"
	echo >>"$(PKGFILE)"

install-locales::
	mkdir -p "$(localedir)"
	for l in `cd src/locales ; ls -d ??_??`; \
	do \
	  mkdir -p "$(localedir)/$$l" ; \
	  cp src/locales/$$l/*.txt "$(localedir)/$$l" ; \
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

