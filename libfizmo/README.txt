
 libfizmo aims to provide a Z-Machine interpreter core library in plain C. The
 goal is to have an interpreter library with as little dependencies as possible
 to allow for easy portability. In order to use it stand-alone, you have to
 invoke the "fizmo_start" function in "src/interpreter/fizmo.c" with a screen
 interface. Check fizmo-console for an example.

 Currently the interpreter is in beta status, which means it seems to run
 all non-version-6 games quite without any known bugs. 

 Dependencies:
 libxml2 for parsing of .ifiction files


 Please send bug reports (or other requests) to fizmo@spellbreaker.org.

 libfizmo was written by Christoph Ender in 2005-2011.

 For the latest version, see http://spellbreaker.org/~chrender/fizmo/


 libfizmo uses the Mersenne twister from
 http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
 as a random number generator implementation:
"A C-program for MT19937, with initialization improved 2002/1/26.
 Coded by Takuji Nishimura and Makoto Matsumoto.
 Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 All rights reserved."
 For full copyright notice see "src/fizmo/mt19937ar.c".


 Currently fizmo is in beta status, meaning it might do unexpected things
 such as stop with an error message, crash or cleesh your machine into a
 frog. There is no warranty of any kind whatsoever and you're entirely on
 your own when running it.

