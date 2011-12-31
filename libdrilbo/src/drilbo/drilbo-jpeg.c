
/* drilbo-jpeg.c
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


#ifndef drilbo_jpeg_c_INCLUDED
#define drilbo_jpeg_c_INCLUDED

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <jpeglib.h>

#include <tools/filesys.h>

#include "drilbo.h"
#include "drilbo-jpeg.h"


z_image* read_zimage_from_jpeg(z_file *in)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int row_buf_size;
  JSAMPARRAY row_buf;
  JSAMPROW jsamplerow;
  z_image *result;
  uint8_t *img_data, *img_data_ptr;
  unsigned int i;
  int j;
  int sample_index;
  int number_of_channels;
  size_t len;
  int result_image_type;

  cinfo.err = jpeg_std_error(&jerr);
  //jerr.error_exit = my_error_exit;

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, fsi->get_stdio_stream(in));
  jpeg_read_header(&cinfo, TRUE);

  if (cinfo.out_color_space == JCS_RGB)
  {
    number_of_channels = 3;
    result_image_type = DRILBO_IMAGE_TYPE_RGB;
  }
  else if (cinfo.out_color_space == JCS_GRAYSCALE)
  {
    number_of_channels = 1;
    result_image_type = DRILBO_IMAGE_TYPE_GRAYSCALE;
  }
  else if (cinfo.out_color_space == JCS_YCbCr)
  {
    /*
    R = Y                + 1.40200 * Cr
    G = Y - 0.34414 * Cb - 0.71414 * Cr
    B = Y + 1.77200 * Cb


    Obsolete:
    yCbCr to RGB
    R = Cr * ( 2-2*C_red) + Y;
    B =Cb * (2-2*C_blue) + Y;
    G = (Y - C_blue * B - C_red * R) / C_green;


    */

    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  else if (cinfo.out_color_space == JCS_CMYK)
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  else if (cinfo.out_color_space == JCS_YCCK)
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  else
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  jpeg_start_decompress(&cinfo);

  row_buf_size = cinfo.output_width * cinfo.output_components;
  row_buf= (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_buf_size, 1);

  len = number_of_channels * cinfo.output_width * cinfo.output_height;

  img_data = (uint8_t*)malloc(len);
  img_data_ptr = img_data;

  while (cinfo.output_scanline < cinfo.output_height)
  {
    jpeg_read_scanlines(&cinfo, row_buf, 1);
    jsamplerow = row_buf[0];

    for (i=0, sample_index=0; i<cinfo.output_width; i++)
    {
      for (j=0; j<number_of_channels; j++)
      {
        *img_data_ptr = jsamplerow[sample_index++];
        img_data_ptr++;
      }
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  result = (z_image*)malloc(sizeof(z_image));

  result->bits_per_sample = BITS_IN_JSAMPLE;
  result->width = cinfo.output_width;
  result->height = cinfo.output_height;
  result->image_type = result_image_type;
  result->data = img_data;

  return result;
}

/*
procedure RGBTOCMYK(R : byte;
G : byte;
B : byte;
var C : byte;
var M : byte;
var Y : byte;
var K : byte);
begin
C := 255 - R;
M := 255 - G;
Y := 255 - B;
if C < M then
K := C else
K := M;
if Y < K then
K := Y;
if k > 0 then begin
c := c - k;
m := m - k;
y := y - k;
end;
end;
*/


void write_zimage_to_jpeg(z_image *image, z_file *out, int color_space)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int row_buf_size;
  JSAMPARRAY row_buf;
  JSAMPROW jsamplerow;
  int sample_index;
  uint32_t i;
  int j;
  uint8_t *img_data_ptr;
  J_COLOR_SPACE jpeg_color_space;

  if (image == NULL)
    return;

  if (color_space == COLORSPACE_JCS_CMYK)
    return;

  if (color_space == COLORSPACE_JCS_YCCK)
    return;

  if (color_space == COLORSPACE_JCS_UNKNOWN)
    return;

  img_data_ptr = image->data;

  if (
      (color_space == COLORSPACE_JCS_RGB)
      ||
      (color_space == COLORSPACE_JCS_GRAYSCALE)
      ||
      (color_space == COLORSPACE_JCS_YCbCr)
     )
  {
    if (color_space == COLORSPACE_JCS_RGB)
      jpeg_color_space = JCS_RGB;
    else if (color_space == COLORSPACE_JCS_GRAYSCALE)
      jpeg_color_space = JCS_GRAYSCALE;
    else if (color_space == COLORSPACE_JCS_YCbCr)
      jpeg_color_space = JCS_YCbCr;
    else
      return;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fsi->get_stdio_stream(out));

    cinfo.image_width = image->width;
    cinfo.image_height = image->height;

    if (image->image_type == DRILBO_IMAGE_TYPE_RGB)
    {
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;

      jpeg_set_defaults(&cinfo);

      jpeg_set_colorspace(&cinfo, jpeg_color_space);

      jpeg_start_compress(&cinfo, TRUE);

      row_buf_size = image->width * cinfo.input_components;
      row_buf= (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_buf_size, 1);
      jsamplerow = row_buf[0];

      while (cinfo.next_scanline < cinfo.image_height)
      {
        //printf("%d\n", cinfo.next_scanline);

        for (i=0, sample_index=0; i<image->width; i++)
        {
          for (j=0; j<cinfo.input_components; j++)
          {
            jsamplerow[sample_index++] = *img_data_ptr;
            img_data_ptr++;
          }
        }

        jpeg_write_scanlines(&cinfo, row_buf, 1);
      }

      jpeg_finish_compress(&cinfo);
      jpeg_destroy_compress(&cinfo);
    }
  }
}

#endif /* drilbo_jpeg_c_INCLUDED */

