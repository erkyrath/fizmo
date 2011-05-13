
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
};

#endif /* screen_cell_interface_h_INCLUDED */

