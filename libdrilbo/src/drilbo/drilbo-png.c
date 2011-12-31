
/* drilbo-png.c
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


#ifndef drilbo_png_c_INCLUDED
#define drilbo_png_c_INCLUDED

#include <stdlib.h>
#include <png.h>

#include <tools/types.h>
#include <tools/filesys.h>

#include "drilbo.h"
#include "drilbo-png.h"


z_image* read_zimage_from_png(z_file *in)
{
  uint8_t header[8];
  int y;
  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  //png_structp png_ptr;
  //png_infop info_ptr;
  //int number_of_passes, interlace_type;
  png_bytep * row_pointers;
  uint8_t *zimage_data, *zimage_ptr;
  z_image *result;
  int bytes_per_pixel;
  png_size_t row_bytes;
  png_color_16 black_background = { 0, 0, 0, 0, 0 };
  png_color_16p image_background;

  fsi->getchars(header, 8, in);
  if (png_sig_cmp(header, 0, 8))
    return NULL;

  png_structp png_ptr = png_create_read_struct(
      PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    return NULL;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,
        (png_infopp)NULL, (png_infopp)NULL);
    return NULL;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
        (png_infopp)NULL);
    return NULL;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    printf("Error reading PNG.\n");
    exit(-1);
  }

  png_init_io(png_ptr, fsi->get_stdio_stream(in));
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

#if PNG_LIBPNG_VER_MAJOR > 1 || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
#else
  width = info_ptr->width;
  height = info_ptr->height;
  color_type = info_ptr->color_type;
  bit_depth = info_ptr->bit_depth;
#endif

  //printf("%d * %d, %dbpp\n", width, height, bit_depth);

  if (png_get_bKGD(png_ptr, info_ptr, &image_background))
    png_set_background(png_ptr, image_background,
        PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
  else
    png_set_background(png_ptr, &black_background,
        PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

  if (color_type == PNG_COLOR_TYPE_GRAY)
    bytes_per_pixel = 1;
  else
    bytes_per_pixel = 3;

  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
  {
#if PNG_LIBPNG_VER_MAJOR > 1 || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
    png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
    png_set_gray_1_2_4_to_8(png_ptr);
#endif
  }

  //if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  //  png_set_tRNS_to_alpha(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  //row_bytes = png_get_rowbytes(png_ptr, info_ptr);
  //printf("suggested-rowbytes: %d\n", row_bytes);

  // For some strange reason, the "png_get_rowbytes" will return the width
  // of image "infocom-brain-ad.png" instead of width * 3, so we have to
  // calculate "row_bytes" on our own:
  row_bytes = width * bytes_per_pixel;

  //printf("rbtes: %d\n", (long)row_bytes);
  zimage_data = malloc((long)row_bytes * height);

  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
  zimage_ptr = zimage_data;
  for (y=0; y<height; y++)
  {
    row_pointers[y] = zimage_ptr;
    zimage_ptr += width * bytes_per_pixel;
  }

  /*
  interlace_type = png_get_interlace_type(png_ptr, info_ptr);
  number_of_passes = png_set_interlace_handling(png_ptr);

  if (interlace_type == PNG_INTERLACE_ADAM7)
    number_of_passes = png_set_interlace_handling(png_ptr);
  */

  png_read_image(png_ptr, row_pointers);

  free(row_pointers);

  result = (z_image*)malloc(sizeof(z_image));

  result->bits_per_sample = 8;
  result->width = width;
  result->height = height;
  result->data = zimage_data;

  if (color_type == PNG_COLOR_TYPE_GRAY)
    result->image_type = DRILBO_IMAGE_TYPE_GRAYSCALE;
  else
    result->image_type = DRILBO_IMAGE_TYPE_RGB;

  return result;
}


/*
void write_zimage_to_png(z_image *image, z_file *out)
{
}
*/


#endif /* drilbo_png_c_INCLUDED */

