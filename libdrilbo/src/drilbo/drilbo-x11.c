
/* drilbo-x11.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2010-2011 Christoph Ender.
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

/*
 *  --- General information ---
 *
 * This file contains all X11-related functionality. In the current state
 * that means that there's one function which displays a supplied zimage
 * on-screen.
 * 
 * At the moment, only TrueColor visuals are supported (it should probably
 * also support Pseudocolor, which, especially for the original infocom
 * images would be quite convenient).
 *
 *
 *  --- Implementation details ---
 *
 * Main challange: Xlib doesn't provide any means of getting callbacks
 * for events. Thus, one has to either poll the event queue -- thus eating
 * up 100% CPU -- or to poll in regular intervals -- hard to implement and
 * makes event response sluggish for high delay or either machine slow when
 * low delay eats up again all CPU time.
 * This implies the use of threads, one thread being the normal main execution
 * and a second thread -- here: "xevent_thead" -- examines the event queue.
 * This is the reason why drilbo requires thread usage. I've chosen pthreads 
 * since fizmo was designed with POSIX compiatiblity in mind.
 *
 * Next problem: X ist not thread safe. Thus we'll do a select() on the
 * display's file descriptor, lock a mutex and only then touch the event
 * queue. After all events have been processed the mutex is freed on the main
 * routine is allowed to execute xlib code. It's important to always lock
 * the X mutex "xevent_mutex" every time xlib code is executed.
 */

#ifndef drilbo_x11_c_INCLUDED
#define drilbo_x11_c_INCLUDED

#include <stdbool.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/Xatom.h>
#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include  "tools/i18n.h"

#include "drilbo-x11.h"
#include "../locales/libdrilbo_locales.h"

#define MIN_SCALED_WIDTH 10
#define MIN_SCALED_HEIGHT 10

static Display *x11_display = NULL;
static int x11_screennumber;
static int x11_defaultdepth;
static int x11_bytesperpixel;
static Screen *x11_screen;
static Visual *x11_visual;
static GC x11_gc;
static XVisualInfo x11_visualinfo;
static int x11_numberofredbits;
static int x11_numberofgreenbits;
static int x11_numberofbluebits;
static int x11_redbitsshiftamount;
static int x11_greenbitsshiftamount;
static int x11_bluebitsshiftamount;
static x11_image_window **x11_image_windows = NULL;
static volatile int space_for_x11_image_windows = 0;
static pthread_t xnextevent_thread;
static pthread_mutex_t xevent_mutex;
static volatile bool xevent_thread_active = false;
static int thread_signalling_pipe[2];
//static int x11_macos = true;


typedef struct
{
  unsigned long   flags;
  unsigned long   functions;
  unsigned long   decorations;
  long            inputMode;
  unsigned long   status;
} Hints;


static x11_image_window_id add_x11_image_window(Window window, int width,
    int height, z_image *zimage, void (*callback)(x11_image_window_id window_id,
      int event))
{
  int i;
  int new_window_index;

  x11_image_window *new_x11_image_window;

  new_window_index = -1;
  for (i=0; i<space_for_x11_image_windows; i++)
  {
    if (x11_image_windows[i] == NULL)
    {
      new_window_index = i;
      break;
    }
  }

  if (new_window_index == -1)
  {
    new_window_index = i;
    space_for_x11_image_windows += 8;

    x11_image_windows = realloc(
        x11_image_windows,
        sizeof(x11_image_window*) * space_for_x11_image_windows);

    for (i++; i<space_for_x11_image_windows; i++)
      x11_image_windows[i] = NULL;
  }

  new_x11_image_window = malloc(sizeof(x11_image_window));

  new_x11_image_window->window = window;
  new_x11_image_window->image_window_id = new_window_index;
  new_x11_image_window->width = width;
  new_x11_image_window->height = height;
  new_x11_image_window->pixmap_stored = false;
  new_x11_image_window->zimage = zimage;
  new_x11_image_window->callback = callback;
  new_x11_image_window->image_aspect_ratio
    = (float)zimage->width / (float)zimage->height;

  new_x11_image_window->exposeEventFound = false;
  new_x11_image_window->resizeRequest.found = false;
  new_x11_image_window->is_mapped = false;

  x11_image_windows[new_window_index] = new_x11_image_window;

  return new_window_index;
}


static x11_image_window *find_x11_image_window(Window window_to_find)
{
  int i;
  x11_image_window *window;

  for (i=0; i<space_for_x11_image_windows; i++)
  {
    window = x11_image_windows[i];
    if ( (window != NULL) && (window->window == window_to_find) )
      return window;
  }

  return NULL;
}


static bool is_system_big_endian()
{
    long one = 1;
    return !(*((char *)(&one)));
}


static XImage *create_XImage_from_zimage(z_image *image)
{
  char *data, *data_ptr;
  uint8_t *src_data = image->data;
  uint32_t rgb_color;
  uint32_t y, x;
  int i;
  uint8_t red, green, blue;

  if (
      (image->image_type != DRILBO_IMAGE_TYPE_RGB)
      &&
      (image->image_type != DRILBO_IMAGE_TYPE_GRAYSCALE)
     )
    return NULL;

  data = malloc(x11_bytesperpixel * image->width * image->height);
  data_ptr = data;

  //printf("create-ximage-from-zimage\n");

  for (y=0; y<image->height; y++)
    for (x=0; x<image->width; x++)
    {
      red = *(src_data++);

      if (image->image_type == DRILBO_IMAGE_TYPE_RGB)
      {
        green = *(src_data++);
        blue = *(src_data++);
      }
      else
      {
        green = red;
        blue = red;
      }

      rgb_color
        = (red >> ((image->bits_per_sample - x11_numberofredbits))
            << x11_redbitsshiftamount)
        | (green >> ((image->bits_per_sample - x11_numberofgreenbits))
            << x11_greenbitsshiftamount)
        | (blue >> ((image->bits_per_sample - x11_numberofbluebits))
            << x11_bluebitsshiftamount);

      if (is_system_big_endian() == true)
      {
        for (i=0; i<x11_bytesperpixel; i++)
          *(data_ptr++) = rgb_color >> ((x11_bytesperpixel - i - 1) * 8);
      }
      else
      {
        for (i=0; i<x11_bytesperpixel; i++)
          *(data_ptr++) = rgb_color >> (i * 8);
      }
    }

  return XCreateImage(
      x11_display,
      x11_visual,
      x11_defaultdepth,
      ZPixmap,
      0,
      data,
      image->width,
      image->height,
      x11_bytesperpixel * 8, //32
      0); //cinfo.output_width * x11_bytesperpixel);
}


static void redraw_image(x11_image_window *window)
{
  int x_space, y_space, x_offset, y_offset;
  int left_x_space, right_x_space, top_y_space, bottom_y_space;

  x_space = window->width - window->pixmap_width;
  y_space = window->height - window->pixmap_height;

  x_offset = x_space / 2;
  y_offset = y_space / 2;

  left_x_space = x_offset;
  right_x_space = x_space - x_offset;

  top_y_space = y_offset;
  bottom_y_space = y_space - y_offset;

  if (left_x_space > 0)
  {
    XFillRectangle(
        x11_display, window->window, x11_gc,
        0,
        0,
        left_x_space,
        window->height);
  }

  if (right_x_space > 0)
  {
    XFillRectangle(
        x11_display, window->window, x11_gc,
        left_x_space +  window->pixmap_width,
        0,
        right_x_space,
        window->height);
  }

  if (top_y_space > 0)
  {
    XFillRectangle(
        x11_display, window->window, x11_gc,
        0,
        0,
        window->width,
        top_y_space);
  }

  if (bottom_y_space > 0)
  {
    XFillRectangle(
        x11_display, window->window, x11_gc,
        0,
        top_y_space + window->pixmap_height,
        window->pixmap_width,
        bottom_y_space);
  }

  XCopyArea(
      x11_display,
      window->pixmap,
      window->window,
      x11_gc,
      0,
      0,
      window->pixmap_width,
      window->pixmap_height,
      x_offset,
      y_offset);
}


static void update_window_pixmap(z_image *zimage, x11_image_window *window)
{
  XImage *ximage;

  ximage = create_XImage_from_zimage(zimage);

  if (window->pixmap_stored == true)
  {
    XFreePixmap(x11_display, window->pixmap);
    window->pixmap_stored = false;
  }

  window->pixmap = XCreatePixmap(
      x11_display,
      window->window,
      ximage->width,
      ximage->height,
      x11_defaultdepth);

  window->pixmap_stored = true;
  window->pixmap_width = zimage->width;
  window->pixmap_height = zimage->height;

  XPutImage(
      x11_display,
      window->pixmap,
      x11_gc,
      ximage,
      0,
      0,
      0,
      0,
      ximage->width,
      ximage->height);

  XDestroyImage(ximage);
}


int close_image_window(x11_image_window_id window_id)
{
  x11_image_window *window;

  if (window_id > space_for_x11_image_windows-1)
    return -1;

  window = x11_image_windows[window_id];
  if (window == NULL)
    return -1;

  if (window->pixmap_stored == true)
  {
    XFreePixmap(x11_display, window->pixmap);
  }

  free_zimage(window->zimage);

  if (window->callback != NULL)
    window->callback(window_id, DRILBO_IMAGE_WINDOW_CLOSED);

  XDestroyWindow(x11_display, window->window);
  XFlush(x11_display);

  x11_image_windows[window_id] = NULL;
  free(window);

  return 0;
}


void *xevent_thead(void *UNUSED(threadid))
{
  XEvent event;
  x11_image_window *window;
  int x11_fd;
  fd_set in_fds;
  int new_width; //, new_height;
  z_image *scaled_zimage;
  int i;
  bool flushNeeded;
  int max_filedes_number_plus_1;
  int select_retval;

  xevent_thread_active = true;

  for (i=0; i<space_for_x11_image_windows; i++)
  {
    if (x11_image_windows[i] != NULL)
    {
      x11_image_windows[i]->exposeEventFound = false;
      x11_image_windows[i]->resizeRequest.found = false;
    }
  }

  // Run the evaluation procedure forever
  for (;;)
  {
    // Create a File Description Set containing x11_fd
    FD_ZERO(&in_fds);
    x11_fd = ConnectionNumber(x11_display);
    FD_SET(x11_fd, &in_fds);
    FD_SET(thread_signalling_pipe[0], &in_fds);

    max_filedes_number_plus_1
      = (x11_fd < thread_signalling_pipe[0]
          ? thread_signalling_pipe[0]
          : x11_fd) + 1;

    select_retval
      = select(max_filedes_number_plus_1, &in_fds, NULL, NULL, NULL);

    if (select_retval > 0)
    {
      if (FD_ISSET(x11_fd, &in_fds))
      {
        // Since X is not thread-safe, we're locking our X mutex to ensure
        // we're the only one interacting with the X server.
        pthread_mutex_lock(&xevent_mutex);

        flushNeeded = false;

        while (XPending(x11_display))
        {
          // We'll now evaluate all events in the pipeline. "MapNotify" requests
          // can be handeled immediately, but all "ConfigureNotify" -- meaning
          // resize -- and "Expose" requests are stored for later execution.
          // This will make sure that only the last resize is actually executed.
          while (XPending(x11_display))
          {
            XNextEvent(x11_display, &event);

            if ((window = find_x11_image_window(event.xany.window)) != NULL)
            {
              if (event.type == MapNotify)
              {
                window->is_mapped = true;
                update_window_pixmap(window->zimage, window);
                redraw_image(window);
                window->exposeEventFound = false;
                flushNeeded = true;
              }
              else if (event.type == Expose)
              {
                window->exposeEventFound = true;
              }
              else if (event.type == ConfigureNotify)
              {
                if (
                    (event.xconfigure.width != window->width)
                    ||
                    (event.xconfigure.height != window->height)
                   )
                {
                  window->resizeRequest.found = true;
                  window->resizeRequest.width = event.xconfigure.width;
                  window->resizeRequest.height = event.xconfigure.height;
                }
              }
              else if (event.type == ReparentNotify)
              {
              }
              else if (event.type == ClientMessage)
              {
                if ((unsigned long)event.xclient.data.l[0]
                    == window->wm_delete_message)
                {
                  close_image_window(window->image_window_id);
                }
                else
                {
                  //printf("Clientmessage: %ld.\n", event.xclient.data.l[0]);
                }
              }
              else
              {
                //printf("Other event: %d\n", event.type);
              }
            }
          }

          // After evaluating and colleting all requests we will now
          // evaluate them.
          for (i=0; i<space_for_x11_image_windows; i++)
          {
            window = x11_image_windows[i];

            if (window == NULL)
              continue;

            if (window->is_mapped == false)
              continue;

            if (window->resizeRequest.found == true)
            {
              /*
                 printf("resize: %d * %d\n",
                 window->resizeRequest.width,
                 window->resizeRequest.height);
               */

              new_width
                = ((float)window->resizeRequest.width
                    / window->image_aspect_ratio
                    > window->resizeRequest.height)

                ? (float)window->resizeRequest.height
                * window->image_aspect_ratio
                : window->resizeRequest.width;

              //printf("newwidth:%d\n", new_width);

              scaled_zimage = scale_zimage_to_width(window->zimage, new_width);

              update_window_pixmap(scaled_zimage, window);

              free_zimage(scaled_zimage);

              window->width = window->resizeRequest.width;
              window->height = window->resizeRequest.height;

              redraw_image(window);

              window->resizeRequest.found = false;
              window->exposeEventFound = false; // additional redraw not needed.
            }
            else if (window->exposeEventFound == true)
            {
              //printf("-> expose\n");

              redraw_image(window);

              flushNeeded = true;
              window->exposeEventFound = false;
            }
          }

          if (flushNeeded == true)
          {
            XFlush(x11_display);
            //XSync(x11_display, False);
            flushNeeded = false;
          }
        }

        // 
        pthread_mutex_unlock(&xevent_mutex);
      }
      else if (FD_ISSET(thread_signalling_pipe[0], &in_fds))
      {
        //printf("Stopping xevent-thread.\n");
        xevent_thread_active = false;
        return NULL;
      }
    }
  }
}


static int x_error_handler(Display *UNUSED(display), XErrorEvent *event)
{
  char message[1024];

  XGetErrorText(x11_display, event->error_code, message, 1024);

  /*
  printf("X-Error: type %d, serial %ld, code %d, request %d, minor %d:%s\n",
      event->type,
      event->serial,
      event->error_code,
      event->request_code,
      event->minor_code,
      message);
  */

  return 0;
}


static void get_x11_bitmask_size_and_shift(uint32_t mask, int *number_of_bits,
    int *bit_shift_amount)
{
  *bit_shift_amount = 0;
  while ((mask & 1) == 0)
  {
    mask >>= 1;
    (*bit_shift_amount)++;
  }

  *number_of_bits = 0;
  while ((mask & 1) == 1)
  {
    mask >>= 1;
    (*number_of_bits)++;
  }
}


int init_x11_display()
{
  int flags;

  if (x11_display != NULL)
    return 0;

  if ((x11_display = XOpenDisplay(NULL)) == NULL)
    return -1;
/*
    i18n_translate_and_exit(
        libdrilbo_module_name,
        i18n_libdrilbo_COULD_NOT_OPEN_X11,
        -1);
*/

  XSetErrorHandler(&x_error_handler);
  pthread_mutex_init(&xevent_mutex, NULL);

  // Create a new signalling pipe. This pipe is used by a select call to
  // detect a thread-stop request from the "end_x11_display" function.
  if (pipe(thread_signalling_pipe) != 0)
    return -1;

  if ((flags = fcntl(thread_signalling_pipe[0], F_GETFL, 0)) == -1)
    return -1;

  if ((fcntl(thread_signalling_pipe[0], F_SETFL, flags|O_NONBLOCK)) == -1)
    return -1;

  pthread_create(&xnextevent_thread, NULL, xevent_thead, NULL);

  x11_screen = DefaultScreenOfDisplay(x11_display);
  x11_screennumber = DefaultScreen(x11_display);
  x11_defaultdepth = DefaultDepth(x11_display, x11_screennumber);

  x11_bytesperpixel = 1;
  while (x11_bytesperpixel * 8 < x11_defaultdepth)
    x11_bytesperpixel *= 2;

  if (!XMatchVisualInfo(
        x11_display,
        x11_screennumber,
        x11_defaultdepth,
        TrueColor,
        &x11_visualinfo))
  {
    //printf ("Cannot get TrueColor Visual!\n");
    exit (EXIT_FAILURE);
  }

  x11_visual = x11_visualinfo.visual;

  get_x11_bitmask_size_and_shift(
      x11_visualinfo.red_mask, 
      &x11_numberofredbits,
      &x11_redbitsshiftamount);

  get_x11_bitmask_size_and_shift(
      x11_visualinfo.green_mask, 
      &x11_numberofgreenbits,
      &x11_greenbitsshiftamount);

  get_x11_bitmask_size_and_shift(
      x11_visualinfo.blue_mask, 
      &x11_numberofbluebits,
      &x11_bluebitsshiftamount);

  x11_gc = XCreateGC(
      x11_display,
      XRootWindow(x11_display, x11_screennumber), //x11_parent,
      0,
      0);

  XSetForeground(
      x11_display,
      x11_gc,
      WhitePixel(x11_display, x11_screennumber));

  return 0;
}


int end_x11_display()
{
  int ret_val;
  unsigned char write_buffer = 0;
  int i;
  x11_image_window *window;

  pthread_mutex_lock(&xevent_mutex);

  do
  {
    ret_val = write(thread_signalling_pipe[1], &write_buffer, 1);

    if ( (ret_val == -1) && (errno != EAGAIN) )
      return -1;
  }
  while ( (ret_val == -1) && (errno == EAGAIN) );

  while (xevent_thread_active != false)
    ;

  close(thread_signalling_pipe[1]);
  close(thread_signalling_pipe[0]);

  for (i=0; i<space_for_x11_image_windows; i++)
  {
    window = x11_image_windows[i];
    if (window != NULL)
      close_image_window(window->image_window_id);
  }

  free(x11_image_windows);

  XFreeGC(x11_display, x11_gc);

  XCloseDisplay(x11_display);
  x11_display = NULL;

  pthread_mutex_unlock(&xevent_mutex);

  return 0;
}


x11_image_window_id display_zimage_on_X11(Window *parent_window,
    z_image *zimage, void (*callback_func)())
{
  Window window;
  Hints hints;
  Atom atom;
  Window parent;
  XSizeHints *xsizehints;
  bool doInline;
  z_image *image_dup, *scaled_zimage;
  x11_image_window_id result;
  x11_image_window *result_window;
  int x, y, tmp_x, tmp_y;
  unsigned parent_width, parent_height, tmp_border, tmp_depth;
  Window tmp_window;
  int scaled_width, scaled_height;
  double scale_factor;

  XSetWindowAttributes attributes;
  unsigned long value_mask;

  if (x11_display == NULL)
  {
    if (init_x11_display() < 0)
      return -1;
  }

  pthread_mutex_lock(&xevent_mutex);

  image_dup = zimage_dup(zimage);

  attributes.event_mask
    = ExposureMask
    | StructureNotifyMask;

  //attributes.bit_gravity = ForgetGravity;

  /*
  Setting a background will make X clear the window on resize, resulting in
  a lot of flicker. Thus, we'll do background erasing outselves later on.

  attributes.background_pixel
    = WhitePixel(x11_display, x11_screennumber);

  value_mask
    = CWBackPixel
    | CWEventMask;
  attributes.border_pixel
    = BlackPixel(x11_display, x11_screennumber);
  value_mask |= CWBorderPixel;
  */

  value_mask = CWEventMask;

  parent
    = parent_window == NULL
    ? XRootWindow(x11_display, x11_screennumber)
    : *parent_window;

  //printf("root: %ld\n", XRootWindow(x11_display, x11_screennumber));
  //printf("Parent window id: %ld\n", parent);

  XGetGeometry(x11_display, parent, &tmp_window, &tmp_x, &tmp_y,
      &parent_width, &parent_height, &tmp_border, &tmp_depth);

  //printf("%d x %d\n", parent_width, parent_height);

  if (image_dup->width > parent_width - 10)
  {
    scaled_width = parent_width - 10;
    scale_factor = (double)scaled_width / (double)image_dup->height;

    if (scaled_width < MIN_SCALED_WIDTH)
    {
      free_zimage(image_dup);
      return -1;
    }

    scaled_height = image_dup->height * scale_factor;
  }
  else
  {
    scaled_height = image_dup->height;
    scaled_width = image_dup->width;
  }

  if ((unsigned)scaled_height > parent_height - 10)
  {
    scaled_height = parent_height - 10;
    scale_factor = (double)scaled_height / (double)image_dup->height;
    scaled_width = (double)image_dup->width * scale_factor;
    if (scaled_height < MIN_SCALED_HEIGHT)
    {
      free_zimage(image_dup);
      return -1;
    }
  }

  if ((unsigned)scaled_width == image_dup->width)
  {
    scaled_zimage = image_dup;
  }
  else
  {
    scaled_zimage = scale_zimage_to_width(image_dup,scaled_width);
    free_zimage(image_dup);

    if (scaled_zimage == NULL)
      return -1;
  }

  if (parent_width > scaled_zimage->width)
    x = (parent_width - scaled_zimage->width) / 2;
  else
    x = 20;

  if (parent_height > scaled_zimage->height)
    y = (parent_height - scaled_zimage->height) / 2;
  else
    y = 20;


  doInline = false;

  window = XCreateWindow(
      x11_display,
      parent,
      x,
      y,
      scaled_zimage->width,
      scaled_zimage->height, // + (x11_macos == true ? 16 : 0),
      0,
      x11_defaultdepth,
      InputOutput,
      x11_visual,
      value_mask,
      &attributes);

  result = add_x11_image_window(
      window,
      scaled_zimage->width,
      scaled_zimage->height,
      scaled_zimage,
      callback_func);
  result_window = x11_image_windows[result];

  if (doInline == true)
  {
    hints.flags = 2;
    hints.functions = 0;
    hints.decorations = 0;
    hints.inputMode = 0;
    hints.status = 0;

    atom = XInternAtom(x11_display, "_MOTIF_WM_HINTS", True);
    // WM_TRANSIENT_FOR ?
    //XSetWMProperties (?)

    XChangeProperty(
        x11_display,
        window,
        atom,
        atom,
        32,
        PropModeReplace,
        (unsigned char *)&hints,
        5);
  }

  xsizehints = XAllocSizeHints();
  xsizehints->min_aspect.x = scaled_zimage->width;
  xsizehints->min_aspect.y = scaled_zimage->height;
  xsizehints->max_aspect.x = scaled_zimage->width;
  xsizehints->max_aspect.y = scaled_zimage->height;
  xsizehints->flags = PAspect;
  atom = XInternAtom(x11_display, "WM_SIZE_HINTS", True);
  XSetWMSizeHints(x11_display, window, xsizehints, atom);

  result_window->wm_delete_message
    = XInternAtom(x11_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(x11_display, window, &result_window->wm_delete_message, 1);

  XMapWindow(x11_display, window);

  XFlush(x11_display);
  pthread_mutex_unlock(&xevent_mutex);

  return result;
}

#endif /* drilbo_x11_c_INCLUDED */

