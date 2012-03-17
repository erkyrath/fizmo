
/* screen_cell_interface.h
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2010-2012 Christoph Ender.
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
 */


#ifndef screen_cell_interface_h_INCLUDED
#define screen_cell_interface_h_INCLUDED

#include "tools/types.h"

#define EVENT_WAS_INPUT             0x1000

#define EVENT_WAS_TIMEOUT           0x2000

#define EVENT_WAS_WINCH             0x3000

#define EVENT_WAS_CODE              0x4000
#define EVENT_WAS_CODE_BACKSPACE    0x4001
#define EVENT_WAS_CODE_DELETE       0x4002
#define EVENT_WAS_CODE_CURSOR_LEFT  0x4003
#define EVENT_WAS_CODE_CURSOR_RIGHT 0x4004
#define EVENT_WAS_CODE_CURSOR_UP    0x4005
#define EVENT_WAS_CODE_CURSOR_DOWN  0x4006
#define EVENT_WAS_CODE_CTRL_A       0x4007
#define EVENT_WAS_CODE_CTRL_E       0x4008
#define EVENT_WAS_CODE_PAGE_UP      0x4009
#define EVENT_WAS_CODE_PAGE_DOWN    0x400A
#define EVENT_WAS_CODE_ESC          0x400B

struct z_screen_cell_interface
{
  void (*goto_yx)(int y, int x);
  void (*z_ucs_output)(z_ucs *z_ucs_output);
  bool (*is_input_timeout_available)();
  void (*turn_on_input);
  void (*turn_off_input);
  int (*get_next_event)(z_ucs *input, int timeout_millis);

  char* (*get_interface_name)();
  bool (*is_colour_available)();
  bool (*is_bold_face_available)();
  bool (*is_italic_available)();
  int (*parse_config_parameter)(char *key, char *value);
  char* (*get_config_value)(char *key);
  char** (*get_config_option_names)();
  void (*link_interface_to_story)(struct z_story *story);
  void (*reset_interface)();
  int (*close_interface)(z_ucs *error_message);
  void (*set_text_style)(z_style text_style);
  void (*set_colour)(z_colour foreground, z_colour background);
  void (*set_font)(z_font font_type);
  void (*output_interface_info)();
  int (*get_screen_width)();
  int (*get_screen_height)();
  void (*update_screen)();
  void (*redraw_screen_from_scratch)();
  void (*copy_area)(int dsty, int dstx, int srcy, int srcx, int height,
      int width);
  void (*clear_to_eol)();
  void (*clear_area)(int startx, int starty, int xsize, int ysize);
  void (*set_cursor_visibility)(bool visible);
  z_colour (*get_default_foreground_colour)();
  z_colour (*get_default_background_colour)();
};

#endif /* screen_cell_interface_h_INCLUDED */

