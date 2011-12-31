
GROFF = groff
GS = gs -dNOPAUSE -dQUIET

all: plain-text html postscript pdf

plain-text: fizmo.txt fizmo-utf8.txt

fizmo.txt: fizmo-ncursesw.6
	$(GROFF) -Tascii -man fizmo-ncursesw.6 | ul -t dumb > fizmo-ncursesw.txt

fizmo-utf8.txt: fizmo-ncursesw.6
	$(GROFF) -Tutf8 -man fizmo-ncursesw.6 | ul -t dumb > fizmo-ncursesw-utf8.txt

html: fizmo.html

fizmo.html: fizmo-ncursesw.6
	groff -Thtml -man fizmo-ncursesw.6 > fizmo-ncursesw.html

postscript: fizmo-a4.ps fizmo-letter.ps

fizmo-a4.ps: fizmo-ncursesw.6
	groff -t -e -mandoc -Tps -dpaper=a4 -P-pa4 fizmo-ncursesw.6 > fizmo-ncursesw-a4.ps

fizmo-letter.ps: fizmo-ncursesw.6
	groff -t -e -mandoc -Tps -dpaper=letter -P-pletter fizmo-ncursesw.6 > fizmo-ncursesw-letter.ps

pdf: fizmo-a4.pdf fizmo-letter.pdf

fizmo-a4.pdf: fizmo-ncursesw.6
	groff -t -e -mandoc -Tps -dpaper=a4 -P-pa4 fizmo-ncursesw.6 | $(GS) -sDEVICE=pdfwrite -sOutputFile=fizmo-ncursesw-a4.pdf

fizmo-letter.pdf: fizmo-ncursesw.6
	groff -t -e -mandoc -Tps -dpaper=letter -P-pletter fizmo-ncursesw.6 | $(GS) -sDEVICE=pdfwrite -sOutputFile=fizmo-ncursesw-letter.pdf

clean:
	rm -f fizmo-ncursesw.txt fizmo-ncursesw-utf8.txt fizmo-ncursesw.html fizmo-ncursesw-a4.ps fizmo-ncursesw-letter.ps fizmo-ncursesw-a4.pdf fizmo-ncursesw-letter.pdf

