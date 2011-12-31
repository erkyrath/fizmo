
.PHONY : clean

include ../../config.mk

ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif

FIZMO_CFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libfizmo)

all: fizmo_ncursesw_locales.o


fizmo_ncursesw_locales.o: fizmo_ncursesw_locales.c
	$(CC) $(CFLAGS) $(FIZMO_CFLAGS) -c fizmo_ncursesw_locales.c

clean:
	rm -f *.o

distclean: clean

