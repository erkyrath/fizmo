
/* drilbo-x11.h
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


#ifndef drilbo_x11_h_INCLUDED
#define drilbo_x11_h_INCLUDED

#include <X11/Xlib.h>

#include "drilbo.h"

#define DRILBO_IMAGE_WINDOW_CLOSED 1


typedef int x11_image_window_id;

typedef struct
{
  // A "x11_image_window" in this context represents a single X11 window whose
  // only purpose is to display a single image.
  // The user is allowed to resize the window for two reasons: Especially
  // when displaying old infocom pictures, the average graphic size of
  // 320x200 -- and often much smaller -- is just a tiny spot on today's
  // much larger displays. Second, there seems to exist no reliable way
  // to enforce a fixed-size window with all window managers (please correct
  // me if this is not true).
  // The "x11_image_window" struct contains all data related to displaying the
  // image in it's current state, the zimage itself, size data and even
  // a reference to the X11 server-side pixmap in which the currently scaled
  // zimage resides for faster redrawing.

  Window window;
  x11_image_window_id image_window_id;
  int width;
  int height;

  z_image *zimage;
  float image_aspect_ratio;
  Pixmap pixmap;
  int pixmap_width;
  int pixmap_height;
  void (*callback)(int x11_image_window_id, int event);

  bool pixmap_stored;
  bool is_mapped;

  bool exposeEventFound;
  struct { bool found; int width; int height; } resizeRequest;
  Atom wm_delete_message;
} x11_image_window;


int init_x11_display();
int end_x11_display();
x11_image_window_id display_zimage_on_X11(Window *parent_window,
    z_image *image, void (*callback_func)());
int close_image_window(x11_image_window_id window_id);


#endif /* drilbo_x11_h_INCLUDED */

