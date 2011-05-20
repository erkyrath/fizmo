
include ../../config.mk

#GLKINCLUDEDIR = ../../../../cheapglk
#GLKLIBDIR = ../../../../cheapglk
#GLKMAKEFILE = Make.cheapglk

GLKINCLUDEDIR = ../../../../glkterm
GLKLIBDIR = ../../../../glkterm
GLKMAKEFILE = Make.glkterm

include $(GLKINCLUDEDIR)/$(GLKMAKEFILE)

CFLAGS += -I$(GLKINCLUDEDIR)

ifneq ($(ENABLE_TRACING),)
override CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(ENABLE_READCHAR_VIA_TERMIOS),)
override CFLAGS += -DENABLE_READCHAR_VIA_TERMIOS=
endif

override CFLAGS += $(shell pkg-config --cflags libfizmo)
LIBS = -L$(GLKLIBDIR) $(GLKLIB) $(LINKLIBS) -lm
LIBS += $(shell pkg-config --libs libfizmo)

fizmo-glk: interface.o unixstrt.o
	$(CC) $(CFLAGS) interface.o unixstrt.o -o fizmo-glk $(LIBS)
	#strip fizmo

interface.o: interface.c
	$(CC) $(CFLAGS) -c interface.c

unixstrt.o: unixstrt.c
	$(CC) $(CFLAGS) -c unixstrt.c

clean:
	rm -f *.o

distclean: clean
	rm -f fizmo-glk

