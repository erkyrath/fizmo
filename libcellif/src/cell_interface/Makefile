
.PHONY : clean

include ../../config.mk

ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif

ifneq ($(ENABLE_TRACING),)
override CFLAGS += -DENABLE_TRACING=
endif

LIBFIZMO_CFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config  \
 --cflags libfizmo)

libcellif.a: cell_interface.o ../locales/libcellif_locales.o
	"$(AR)" rcs libcellif.a cell_interface.o ../locales/libcellif_locales.o

cell_interface.o: cell_interface.h cell_interface.c
	"$(CC)" $(CFLAGS) -c cell_interface.c $(LIBFIZMO_CFLAGS)

../locales/libcellif_locales.o::
	cd ../locales ; make

clean:
	rm -f *.o

distclean: clean
	rm -f libcellif.a

