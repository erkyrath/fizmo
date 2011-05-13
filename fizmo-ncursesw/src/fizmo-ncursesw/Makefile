
.PHONY : all clean distclean

include ../../config.mk

CC = gcc -Wall

CFLAGS = -g -O2 $(SOUND_INTERFACE_CFLAGS)
CFLAGS += $(shell pkg-config --cflags libfizmo)
CFLAGS += $(shell pkg-config --cflags libcellif)
CFLAGS += $(NCURSESW_PKG_CFLAGS) $(NCURSESW_NONPKG_CFLAGS)

LIBS = -lm
LIBS += $(shell pkg-config --libs libfizmo)
LIBS += $(shell pkg-config --libs libcellif)
LIBS += $(NCURSESW_PKG_LIBS) $(NCURSESW_NONPKG_LIBS)


ifneq ($(ENABLE_GDB_SYMBOLS),)
  CFLAGS += -g
endif

ifneq ($(ENABLE_TRACING),)
  CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(SOUND_INTERFACE_NAME),)
ifneq ($(SOUND_INTERFACE_STRUCT_NAME),)
ifneq ($(SOUND_INTERFACE_INCLUDE_FILE),)
  SNDIF_VAR_NAME_CFLAGS_PKG = $(SOUND_INTERFACE_CONFIGNAME)_PKG_CFLAGS
  SNDIF_VAR_NAME_CFLAGS_NONPKG = $(SOUND_INTERFACE_CONFIGNAME)_NONPKG_CFLAGS
  CFLAGS += $($(SNDIF_VAR_NAME_CFLAGS_PKG)) $($(SNDIF_VAR_NAME_CFLAGS_NONPKG))
  CFLAGS += $(shell pkg-config --cflags $(SOUND_INTERFACE_NAME))
  LIBS += $(shell pkg-config --libs $(SOUND_INTERFACE_NAME))
  SOUND_FLAGS=-DSOUND_INTERFACE_STRUCT_NAME="$(SOUND_INTERFACE_STRUCT_NAME)" \
	-DSOUND_INTERFACE_INCLUDE_FILE="<$(SOUND_INTERFACE_INCLUDE_FILE)>"
endif
endif
endif

#LIBS = $(LIBCELLIF_LIBS) $(LIBFIZMO_LIBS) $(SOUND_INTERFACE_LIBS) -lncursesw
FLAGS =

ifneq ($(ENABLE_X11_IMAGES),)
FLAGS += -DENABLE_X11_IMAGES
CFLAGS += $(shell pkg-config --cflags libdrilbo)
LIBS += $(shell pkg-config --libs libdrilbo)
endif


all: fizmo-ncursesw

fizmo-ncursesw: fizmo-ncursesw.o ../locales/fizmo_ncursesw_locales.o
	$(CC) $(CFLAGS) fizmo-ncursesw.o ../locales/fizmo_ncursesw_locales.o $(LIBS) -o fizmo-ncursesw

../locales/fizmo_ncursesw_locales.o::
	cd ../locales ; make

fizmo-ncursesw.o: fizmo-ncursesw.c
	$(CC) $(CFLAGS) $(FLAGS) $(SOUND_FLAGS) -c fizmo-ncursesw.c

clean:
	rm -f *.o
	cd ../locales ; make clean

distclean: clean
	rm -f fizmo-ncursesw
	cd ../locales ; make distclean

