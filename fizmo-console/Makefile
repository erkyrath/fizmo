
.PHONY : all install clean distclean

include config.mk


all: src/fizmo-console/fizmo-console

src/fizmo-console/fizmo-console::
	cd src/fizmo-console ; make
	cp src/fizmo-console/fizmo-console .

install: src/fizmo-console/fizmo-console
	mkdir -p "$(bindir)"
	cp src/fizmo-console/fizmo-console "$(bindir)"
	mkdir -p "$(mandir)/man6"
	cp src/man/fizmo-console.6 "$(mandir)/man6"

clean:
	cd src/fizmo-console ; make clean

distclean: clean
	rm -f fizmo-console
	cd src/fizmo-console ; make distclean

