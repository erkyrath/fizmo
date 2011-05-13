
 Fizmo is a Z-Machine interpreter ("Fizmo Interprets Z-Machine Opcodes") which
 allows you to run Infocom- and most other Z-Machine based games -- except
 version 6 -- on POSIX-like systems which provide a ncursesw (note the "w")
 library. It has been successfully compiled on Debian based Linux, Mac OS X
 (with MacPorts providing ncursesw) and Windows (using Cygwin and a self-
 compiled ncursesw library).

 About Infocom and interactive fiction in general, see the "New to IF" section
 at http://www.ifarchive.org.

 To download Z-Machine games, see
 http://www.ifarchive.org/indexes/if-archiveXgamesXzcode.html

 Please send bug reports (or other requests) to fizmo@spellbreaker.org.

 Fizmo was written by Christoph Ender in 2005-2010.

 The latest version should be available from 
 http://spellbreaker.org/~chrender/fizmo

 ------------------------------------------------------------------------------

 libfizmo uses the Mersenne twister from
 http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
 as a random number generator implementation:
"A C-program for MT19937, with initialization improved 2002/1/26.
 Coded by Takuji Nishimura and Makoto Matsumoto.
 Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 All rights reserved."
 For full copyright notice see "src/fizmo/mt19937ar.c".

 ------------------------------------------------------------------------------

 libfizmo includes the following hyphenation patterns from the offo ("Objects
 For Formatting Objects") project from sourceforge, available from
 http://offo.sourceforge.net:

  - en_US.xml, under D.E. Knuth's TeX license.
  - de.xml, under LaTeX Project Public License.

 ------------------------------------------------------------------------------

 Please note:
 Currently fizmo is in beta status, meaning it might do unexpected things
 such as stop with an error message, crash or cleesh your machine into a
 frog. There is no warranty of any kind whatsoever and you're entirely on
 your own when running it.


