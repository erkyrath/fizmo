
#ifndef types_c_INCLUDED
#define types_c_INCLUDED

#include <strings.h>
#include "types.h"

char* z_colour_names[] = { "black", "red", "green", "yellow", "blue",
  "magenta", "cyan", "white" };

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

