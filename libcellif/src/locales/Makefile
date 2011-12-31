
include ../../config.mk


ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif

LIBFIZMO_CFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libfizmo)


.PHONY : clean

all: libcellif_locales.o

libcellif_locales.o: libcellif_locales.c
	"$(CC)" $(CFLAGS) $(LIBFIZMO_CFLAGS) -c libcellif_locales.c

clean:
	rm -f *.o

