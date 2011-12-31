
.PHONY : clean

include ../../config.mk

ifneq ($(ENABLE_GDB_SYMBOLS),)
CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
CFLAGS += -O2
endif

ifneq ($(ENABLE_TRACING),)
CFLAGS += -DENABLE_TRACING=
endif

LIBFIZMO_CFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config  \
 --cflags libfizmo)

CFLAGS += $(LIBFIZMO_CFLAGS)
CFLAGS += $(LIBGLK_NONPKG_CFLAGS)

libglkif.a: glk_interface.o glk_screen_if.o glk_filesys_if.o glk_blorb_if.o ../locales/libglkif_locales.o
	$(AR) rcs libglkif.a glk_interface.o glk_screen_if.o glk_filesys_if.o glk_blorb_if.o ../locales/libglkif_locales.o

glk_interface.o: glk_interface.h glk_interface.c glk_screen_if.h glk_filesys_if.h glk_blorb_if.h
	$(CC) $(CFLAGS) -c glk_interface.c

glk_screen_if.o: glk_screen_if.h glk_screen_if.c
	$(CC) $(CFLAGS) -c glk_screen_if.c

glk_filesys_if.o: glk_filesys_if.h glk_filesys_if.c
	$(CC) $(CFLAGS) -c glk_filesys_if.c

glk_blorb_if.o: glk_blorb_if.h glk_blorb_if.c
	$(CC) $(CFLAGS) -c glk_blorb_if.c

../locales/libglkif_locales.o::
	cd ../locales ; make


clean:
	rm -f *.o

distclean: clean
	rm -f libglkif.a

