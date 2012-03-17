
/* types.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2009-2012 Christoph Ender.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Why tracelog at all? Some errors, for example when resizing screens or
 * other procedures, simply do not occur when running under gdb, so an
 * extra debug method is very welcome. Furthermore, the tracelog also
 * allows to get a very detailed log of a user's error without having to
 * install or explain a debugger fist.
 *
 */


#ifndef types_c_INCLUDED
#define types_c_INCLUDED

#include <strings.h>
#include "types.h"

char* z_colour_names[] = {
  "current", "default", "black", "red", "green", "yellow", "blue",
  "magenta", "cyan", "white", "msdos-grey", "amiga-grey", "medium-grey",
  "drak grey" };

bool is_regular_z_colour(z_colour colour)
{
  if (
      (colour == Z_COLOUR_BLACK)
      ||
      (colour == Z_COLOUR_RED)
      ||
      (colour == Z_COLOUR_GREEN)
      ||
      (colour == Z_COLOUR_YELLOW)
      ||
      (colour == Z_COLOUR_BLUE)
      ||
      (colour == Z_COLOUR_MAGENTA)
      ||
      (colour == Z_COLOUR_CYAN)
      ||
      (colour == Z_COLOUR_WHITE)
      ||
      (colour == Z_COLOUR_MSDOS_DARKISH_GREY)
      ||
      (colour == Z_COLOUR_AMIGA_LIGHT_GREY)
      ||
      (colour == Z_COLOUR_MEDIUM_GREY)
      ||
      (colour == Z_COLOUR_DARK_GREY)
     )
    return true;
  else
    return false;
}


short color_name_to_z_colour(char *colour_name)
{
  if      (colour_name == NULL)                     { return -1;               }
  else if (strcasecmp(colour_name, "black") == 0)   { return Z_COLOUR_BLACK;   }
  else if (strcasecmp(colour_name, "red") == 0)     { return Z_COLOUR_RED;     }
  else if (strcasecmp(colour_name, "green") == 0)   { return Z_COLOUR_GREEN;   }
  else if (strcasecmp(colour_name, "yellow") == 0)  { return Z_COLOUR_YELLOW;  }
  else if (strcasecmp(colour_name, "blue") == 0)    { return Z_COLOUR_BLUE;    }
  else if (strcasecmp(colour_name, "magenta") == 0) { return Z_COLOUR_MAGENTA; }
  else if (strcasecmp(colour_name, "cyan") == 0)    { return Z_COLOUR_CYAN;    }
  else if (strcasecmp(colour_name, "white") == 0)   { return Z_COLOUR_WHITE;   }
  else                                              { return -1;               }
}


#endif /* types_c_INCLUDED */

