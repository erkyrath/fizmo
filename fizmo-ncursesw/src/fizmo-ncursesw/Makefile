
.PHONY : all clean distclean

include ../../config.mk


override CFLAGS += -g -O2 $(SOUND_INTERFACE_CFLAGS)
override CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libfizmo)
override CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libcellif)
override CFLAGS += $(NCURSESW_PKG_CFLAGS) $(NCURSESW_NONPKG_CFLAGS)

LIBS = -lm
LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs libfizmo)
LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs libcellif)
LIBS += $(NCURSESW_PKG_LIBS) $(NCURSESW_NONPKG_LIBS)


ifneq ($(ENABLE_GDB_SYMBOLS),)
  override CFLAGS += -g
endif

ifneq ($(ENABLE_TRACING),)
  override CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(SOUND_INTERFACE_NAME),)
ifneq ($(SOUND_INTERFACE_STRUCT_NAME),)
ifneq ($(SOUND_INTERFACE_INCLUDE_FILE),)
  SNDIF_VAR_NAME_CFLAGS_PKG = $(SOUND_INTERFACE_CONFIGNAME)_PKG_CFLAGS
  SNDIF_VAR_NAME_CFLAGS_NONPKG = $(SOUND_INTERFACE_CONFIGNAME)_NONPKG_CFLAGS
  override CFLAGS += $($(SNDIF_VAR_NAME_CFLAGS_PKG)) $($(SNDIF_VAR_NAME_CFLAGS_NONPKG))
  override CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags $(SOUND_INTERFACE_NAME))
  LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --libs $(SOUND_INTERFACE_NAME))
  SOUND_FLAGS=-DSOUND_INTERFACE_STRUCT_NAME="$(SOUND_INTERFACE_STRUCT_NAME)" \
	-DSOUND_INTERFACE_INCLUDE_FILE="<$(SOUND_INTERFACE_INCLUDE_FILE)>"
endif
endif
endif

#LIBS = $(LIBCELLIF_LIBS) $(LIBFIZMO_LIBS) $(SOUND_INTERFACE_LIBS) -lncursesw
FLAGS =

ifneq ($(ENABLE_X11_IMAGES),)
FLAGS += -DENABLE_X11_IMAGES
override CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libdrilbo)
LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --libs libdrilbo)
endif

ifneq ($(DISABLE_FILELIST),)
override CFLAGS += -DDISABLE_FILELIST=
endif

ifneq ($(DISABLE_CONFIGFILES),)
override CFLAGS += -DDISABLE_CONFIGFILES=
endif


all: fizmo-ncursesw

fizmo-ncursesw: fizmo-ncursesw.o ../locales/fizmo_ncursesw_locales.o
	$(CC) $(LDFLAGS) fizmo-ncursesw.o ../locales/fizmo_ncursesw_locales.o $(LIBS) -o fizmo-ncursesw

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

