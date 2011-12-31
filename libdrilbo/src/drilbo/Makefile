
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

FIZMO_CFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libfizmo)

DRILBO_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --libs libfizmo)
DRILBO_LIB_OBJECTS = drilbo.o drilbo-mg1.o drilbo-ppm.o \
 ../locales/libdrilbo_locales.o
DRILBO_TEST_FLAGS =


ifneq ($(DRILBO_ENABLE_X11),)
 #DRILBO_CFLAGS += $(DRILBO_PKG_X11_CFLAGS) $(DRILBO_NONPKG_X11_CFLAGS)
  DRILBO_LIBS += $(DRILBO_PKG_X11_LIBS) $(DRILBO_NONPKG_X11_LIBS)
 #DRILBO_LIBS += -lX11 -lpthread
  DRILBO_TEST_FLAGS += -DTEST_X11
  DRILBO_LIB_OBJECTS += drilbo-x11.o
endif # DRILBO_ENABLE_X11

ifneq ($(DRILBO_ENABLE_JPG),)
  DRILBO_CFLAGS += -DENABLE_JPG
  DRILBO_LIBS += $(DRILBO_PKG_LIBJPEG_LIBS) $(DRILBO_NONPKG_LIBJPEG_LIBS)
  DRILBO_TEST_FLAGS += -DTEST_JPEG
  DRILBO_LIB_OBJECTS += drilbo-jpeg.o
endif # DRILBO_ENABLE_JPG

ifneq ($(DRILBO_ENABLE_PNG),)
  DRILBO_CFLAGS += -DENABLE_PNG
  DRILBO_LIBS += $(DRILBO_PKG_LIBPNG_LIBS) $(DRILBO_NONPKG_LIBPNG_LIBS)
  DRILBO_TEST_FLAGS += -DTEST_PNG
  DRILBO_LIB_OBJECTS += drilbo-png.o
endif # DRILBO_ENABLE_PNG



all: libdrilbo.a

test: drilbo-test
	./drilbo-test

../locales/libdrilbo_locales.o::
	cd ../locales ; make libdrilbo_locales.o

libdrilbo.a: $(DRILBO_LIB_OBJECTS)
	"$(AR)" rcs libdrilbo.a $(DRILBO_LIB_OBJECTS)

drilbo.o: drilbo.h drilbo.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) $(DRILBO_CFLAGS) -c drilbo.c

drilbo-ppm.o: drilbo-ppm.h drilbo-ppm.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) -c drilbo-ppm.c

drilbo-mg1.o: drilbo-mg1.h drilbo-mg1.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) -c drilbo-mg1.c

drilbo-jpeg.o:drilbo.h drilbo-jpeg.h drilbo-jpeg.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) $(DRILBO_PKG_LIBJPEG_CFLAGS) $(DRILBO_NONPKG_LIBJPEG_CFLAGS) -c drilbo-jpeg.c

drilbo-png.o: drilbo.h drilbo-png.h drilbo-png.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) $(DRILBO_PKG_LIBPNG_CFLAGS) $(DRILBO_NONPKG_LIBPNG_CFLAGS) -c drilbo-png.c

drilbo-x11.o: drilbo-x11.h drilbo-x11.c
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) $(DRILBO_PKG_X11_CFLAGS) $(DRILBO_NONPKG_X11_CFLAGS) -c drilbo-x11.c

drilbo-test: libdrilbo.a drilbo-test.o
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) drilbo.o drilbo-test.o -L. -ldrilbo $(DRILBO_LIBS) -o drilbo-test

drilbo-test.o: drilbo-test.c libdrilbo.a
	"$(CC)" $(CFLAGS) $(FIZMO_CFLAGS) $(DRILBO_TEST_FLAGS) $(DRILBO_PKG_LIBJPEG_CFLAGS) $(DRILBO_NONPKG_LIBJPEG_CFLAGS) $(DRILBO_PKG_LIBPNG_CFLAGS) $(DRILBO_NONPKG_LIBPNG_CFLAGS) $(DRILBO_PKG_X11_CFLAGS) $(DRILBO_NONPKG_X11_CFLAGS) -c drilbo-test.c

clean:
	cd ../locales ; make clean
	rm -f *.o ad-RGB.jpg ad-YCbCr.jpg ad-Grayscale.jpg shogun.ppm tracelog.txt

distclean: clean
	cd ../locales ; make distclean
	rm -f libdrilbo.a

