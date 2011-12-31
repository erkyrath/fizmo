
/* drilbo-test.c
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


#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include <tools/tracelog.h>
#include <tools/i18n.h>
#include <tools/z_ucs.h>
#include <tools/filesys.h>

#include "drilbo.h"
#include "drilbo-ppm.h"

#include "drilbo-jpeg.h"

#ifdef TEST_PNG
#include "drilbo-png.h"
#endif // TEST_PNG

#ifdef TEST_X11
#include "drilbo-x11.h"
#endif // TEST_X11

#include "drilbo-mg1.h"


#define DRILBO_TEST_VERSION "0.1.0-dev"

#define STREAM_BUFSIZE 128

static const char *png_ppm_out_file_name = "infocom-brain-ad.ppm";
static const char *mg1_ppm_out_file_name = "shogun.ppm";

static const char *jpeg_in_file_name = "../test/infocom-brain-ad.jpg";
static const char *jpeg_out_file_rgb_name = "ad-RGB.jpg";
static const char *jpeg_out_file_ycbcr_name = "ad-YCbCr.jpg";
static const char *jpeg_out_file_grayscale_name = "ad-Grayscale.jpg";

static const char *jpeg_grayscale_in_file_name = "../test/zork-poster-grayscale.jpg";

static const char *png_in_file_name = "../test/infocom-brain-ad.png";
static const char *mg1_file_name = "../test/Shogun.mg1";
#ifdef TEST_X11
static x11_image_window_id image_window_id;
static int signalling_pipe[2];
  fd_set in_fds;
  int max_filedes_number_plus_1;
  int select_retval;
  unsigned int read_buf[1];
  int ret_val;
#endif // TEST_X11



void callback_func(x11_image_window_id window_id, int event)
{
  int ret_val;
  unsigned char write_buffer = 0;

  if (image_window_id == window_id)
  {
    if (event == DRILBO_IMAGE_WINDOW_CLOSED)
    {
      do
      {
        ret_val = write(signalling_pipe[1], &write_buffer, 1);

        if ( (ret_val == -1) && (errno != EAGAIN) )
          return;
      }
      while ( (ret_val == -1) && (errno == EAGAIN) );
    }
  }
}


int i18n_test_stream_output(z_ucs *output)
{
  int len;
  char buf[STREAM_BUFSIZE];

  while (*output != 0)
  {
    len = zucs_string_to_utf8_string(buf, &output, STREAM_BUFSIZE);
    fwrite(buf, 1, len-1, stdout);
  }

  printf("X\nX\n");

  return 0;
}


int setup_callback()
{
  int flags;

  if (pipe(signalling_pipe) != 0)
    return -1;

  if ((flags = fcntl(signalling_pipe[0], F_GETFL, 0)) == -1)
    return -1;

  if ((fcntl(signalling_pipe[0], F_SETFL, flags|O_NONBLOCK)) == -1)
    return -1;

  if ((flags = fcntl(STDIN_FILENO, F_GETFL, 0)) == -1)
    return -1;

  if ((fcntl(STDIN_FILENO, F_SETFL, flags|O_NONBLOCK)) == -1)
    return -1;

  return 0;
}


int wait_for_callback()
{
  for (;;)
  {
    FD_ZERO(&in_fds);
    FD_SET(STDIN_FILENO, &in_fds);
    FD_SET(signalling_pipe[0], &in_fds);

    max_filedes_number_plus_1
      = (STDIN_FILENO < signalling_pipe[0]
          ? signalling_pipe[0]
          : STDIN_FILENO) + 1;

    select_retval
      = select(max_filedes_number_plus_1, &in_fds, NULL, NULL, NULL);

    if (select_retval > 0)
    {
      if (FD_ISSET(STDIN_FILENO, &in_fds))
      {
        do
        {
          ret_val = read(STDIN_FILENO, &read_buf, 1);
        }
        while (ret_val > 0);

        break;
      }
      else if (FD_ISSET(signalling_pipe[0], &in_fds))
      {
        do
        {
          ret_val = read(signalling_pipe[0], &read_buf, 1);

          if ( (ret_val == -1) && (errno != EAGAIN) )
          {
            printf("ret_val:%d\n", ret_val);
            return -1;
          }
        }
        while ( (ret_val == -1) && (errno == EAGAIN) );

        break;
      }
    }
  }

  return 0;
}


int main(int UNUSED(argc), char *UNUSED(argv[]))
{
  z_file *in, *out;
  z_image *zork_poster;
  int nof_mg1_images;
  uint16_t *mg1_image_numbers;
  int image_index;
  z_image *mg1_image;
#ifdef TEST_JPEG
  z_image *brain_ad_jpeg, *scaled_brain_ad_jpeg;
#endif // TEST_JPEG
#ifdef TEST_JPEG
  z_image *brain_ad_png;
#endif // TEST_JPEG
#ifdef TEST_X11
  char *env_window_id;
  XID window_id;
#endif // TEST_X11
  //z_ucs *search_path;

#ifdef ENABLE_TRACING
  turn_on_trace();
#endif // ENABLE_TRACING

  /*
  if ((search_path = dup_latin1_string_to_zucs_string("../locales")) == NULL)
  {
    fprintf(stderr, "Could not duplicate search path.\n");
    return -1;
  }
  */

  register_i18n_stream_output_function(&i18n_test_stream_output);

  //if (set_i18n_search_path(search_path) != 0)
  if (set_i18n_search_path("../locales") != 0)
  {
    fprintf(stderr, "Could not set search path.\n");
    return -2;
  }

  printf("\ndrilbo-test v%s\n\n", DRILBO_TEST_VERSION);

  printf("Starting mg1-test.\n");

  printf("Loading \"%s\".\n", mg1_file_name);
  if (init_mg1_graphics(mg1_file_name) != 0)
    return -1;

  nof_mg1_images = get_number_of_mg1_images();

  printf("%d images in \"%s\".\n",
      nof_mg1_images,
      mg1_file_name);

  printf("Image numbers: ");
  if ((mg1_image_numbers = get_all_picture_numbers()) == NULL)
    return -1;
  for (image_index = 0; image_index < nof_mg1_images; image_index++)
  {
    if (image_index != 0)
      printf(", ");
    printf("%d", mg1_image_numbers[image_index]);
  }
  printf("\n");
  free(mg1_image_numbers);

  mg1_image = get_picture(1);

  if (mg1_image == NULL)
    return -1;

  out = fsi->openfile(mg1_ppm_out_file_name, FILETYPE_DATA, FILEACCESS_WRITE);
  write_zimage_to_ppm(mg1_image, out);
  fsi->closefile(out);

  end_mg1_graphics();

#ifdef TEST_JPEG

  printf("\nStarting jpg-test.\n");

  printf("Loading JPEG file \"%s\" ...\n", jpeg_in_file_name);
  in = fsi->openfile(jpeg_in_file_name, FILETYPE_DATA, FILEACCESS_READ);
  brain_ad_jpeg = read_zimage_from_jpeg(in);
  fsi->closefile(in);

  scaled_brain_ad_jpeg = scale_zimage_to_width(brain_ad_jpeg, 800);

  printf("Writing RGB JPEG file \"%s\" ...\n", jpeg_out_file_rgb_name);
  out = fsi->openfile(jpeg_out_file_rgb_name, FILETYPE_DATA, FILEACCESS_WRITE);
  //write_zimage_to_jpeg(brain_ad, out, JCS_RGB);
  write_zimage_to_jpeg(scaled_brain_ad_jpeg, out, JCS_RGB);
  fsi->closefile(out);

  printf("Writing YCbCr JPEG file \"%s\" ...\n", jpeg_out_file_ycbcr_name);
  out = fsi->openfile(jpeg_out_file_ycbcr_name, FILETYPE_DATA,
      FILEACCESS_WRITE);
  write_zimage_to_jpeg(brain_ad_jpeg, out, JCS_YCbCr);
  fsi->closefile(out);

  printf("Writing Grayscale JPEG file \"%s\" ...\n",
      jpeg_out_file_grayscale_name);
  out = fsi->openfile(jpeg_out_file_grayscale_name, FILETYPE_DATA,
      FILEACCESS_WRITE);
  write_zimage_to_jpeg(brain_ad_jpeg, out, JCS_GRAYSCALE);
  fsi->closefile(out);

  printf("Loading Grayscale JPEG file \"%s\" ...\n",
      jpeg_grayscale_in_file_name);
  in = fsi->openfile(jpeg_grayscale_in_file_name, FILETYPE_DATA, FILEACCESS_READ);
  zork_poster = read_zimage_from_jpeg(in);
  fsi->closefile(in);

#endif // TEST_JPEG

#ifdef TEST_PNG

  printf("Starting png-test.\n");

  printf("Loading PNG file \"%s\" ...\n", png_in_file_name);
  in = fsi->openfile(png_in_file_name, FILETYPE_DATA, FILEACCESS_READ);
  brain_ad_png = read_zimage_from_png(in);
  fsi->closefile(in);

  out = fsi->openfile(png_ppm_out_file_name, FILETYPE_DATA, FILEACCESS_WRITE);
  write_zimage_to_ppm(brain_ad_png, out);
  fsi->closefile(out);

  /*
  printf("Loading PNG file \"%s\" ...\n", png_in_file_name);
  in = fsi->openfile(png_in_file_name, FILETYPE_DATA, FILEACCESS_READ);
  brain_ad_png = read_zimage_from_png(in);
  fsi->closefile(in);
  */

#endif // TEST_PNG

#ifdef TEST_X11

  printf("\nStarting X11-test.\n");

  printf("Displaying X11 image window ...\n");
  printf("Close window or press Enter to continue.\n");
  setup_callback();
  image_window_id = display_zimage_on_X11(NULL, mg1_image, &callback_func);
  wait_for_callback();
  close_image_window(image_window_id);

  printf("Trying to display X11 image inlined in current (terminal) ");
  printf("window ...\n");
  env_window_id = getenv("WINDOWID");
  if (env_window_id != NULL)
  {
    //printf("window id: %s\n", env_window_id);
    window_id = atol(env_window_id);
    //printf("%ld\n", window_id);
    setup_callback();
    image_window_id = display_zimage_on_X11(&window_id, mg1_image,
        &callback_func);
    wait_for_callback();
    end_x11_display();
  }
  else
  {
    printf("getenv(\"WINDOWID\") return null, probably not running X.\n");
  }

  //XID window_id;
  //image_window_id = display_zimage_on_X11(zork_poster, &callback_func);
  //image_window_id = display_zimage_on_X11(brain_ad_jpg, &callback_func);
  //wait_for_enter();

#endif

  //free_zimage(brain_ad);

  printf("\nTest finished.\n\n");

  return 0;
}

