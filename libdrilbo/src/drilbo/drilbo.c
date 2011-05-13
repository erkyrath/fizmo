
/* drilbo.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2009-2010 Christoph Ender.
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


#ifndef drilbo_c_INCLUDED
#define drilbo_c_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drilbo.h"


/*
static z_image *resizeBilinear_RGB(z_image *image, int dest_width,
    int dest_height)
{
  int ul, ur, ll, lr, x, y;
  float x_ratio = ((float)(image->width - 1)) / dest_width;
  float y_ratio = ((float)(image->height - 1)) / dest_height;
  float x_right_weight, y_bottom_weight;
  int offset, top_left;
  uint8_t *dest;
  int i, j, k;
  int row_byte_size = image->width * 3;
  z_image *result;

  if (image->bits_per_sample != 8)
    return NULL;
  
  if (image->image_type != DRILBO_IMAGE_TYPE_RGB)
    return NULL;

  dest = malloc(dest_width * dest_height * 3);

  result = (z_image*)malloc(sizeof(z_image));

  result->bits_per_sample = 8;
  result->width = dest_width;
  result->height = dest_height;
  result->image_type = DRILBO_IMAGE_TYPE_RGB;
  result->data = dest;

  offset = 0;

  // Iterate over all output pixels
  for (i=0; i<dest_height; i++)
    for (j=0; j<dest_width; j++)

      // Iterate over red, green and blue for every pixel
      for (k=0; k<3; k++)
      {
        // Compute top left source image equivalent address
        x = (int)(x_ratio * j);
        y = (int)(y_ratio * i);
        top_left = y*row_byte_size + x*3 + k;

        // Read four-pixel-block originating from "top_left":
        ul = image->data[top_left];
        ur = image->data[top_left + 3];
        ll = image->data[top_left + row_byte_size];
        lr = image->data[top_left + row_byte_size + 3];

        // Calculate proximity of dest pixel to source pixels
        x_right_weight = (x_ratio * j) - x;
        y_bottom_weight = (y_ratio * i) - y;

        // Evaluate dest pixel value
        dest[offset++]
          = ul * (1-x_right_weight ) * (1-y_bottom_weight)
          + ur * (  x_right_weight ) * (1-y_bottom_weight)
          + ll * (  y_bottom_weight) * (1-x_right_weight )
          + lr * (  x_right_weight ) * (  y_bottom_weight);
      }

  return result;
}
*/


static z_image *resizeBilinear(z_image *image, int dest_width,
    int dest_height)
{
  int ul, ur, ll, lr, x, y;
  float x_ratio = ((float)(image->width - 1)) / dest_width;
  float y_ratio = ((float)(image->height - 1)) / dest_height;
  float x_right_weight, y_bottom_weight;
  int offset, top_left;
  uint8_t *dest;
  int i, j, k;
  int row_byte_size = image->width * 3;
  z_image *result;
  int bytes_per_pixel;

  if (image->bits_per_sample != 8)
    return NULL;
  
  if (image->image_type == DRILBO_IMAGE_TYPE_RGB)
    bytes_per_pixel = 3;
  else if (image->image_type == DRILBO_IMAGE_TYPE_GRAYSCALE)
    bytes_per_pixel = 1;
  else
    return NULL;

  row_byte_size = image->width * bytes_per_pixel;

  dest = malloc(dest_width * dest_height * bytes_per_pixel);

  result = (z_image*)malloc(sizeof(z_image));

  result->bits_per_sample = 8;
  result->width = dest_width;
  result->height = dest_height;
  result->image_type = image->image_type;
  result->data = dest;

  offset = 0;

  // Iterate over all output pixels
  for (i=0; i<dest_height; i++)
    for (j=0; j<dest_width; j++)

      // Iterate over every pixel
      for (k=0; k<bytes_per_pixel; k++)
      {
        // Compute top left source image equivalent address
        x = (int)(x_ratio * j);
        y = (int)(y_ratio * i);
        top_left = y*row_byte_size + x*bytes_per_pixel + k;

        // Read four-pixel-block originating from "top_left":
        ul = image->data[top_left];
        ur = image->data[top_left + bytes_per_pixel];
        ll = image->data[top_left + row_byte_size];
        lr = image->data[top_left + row_byte_size + bytes_per_pixel];

        // Calculate proximity of dest pixel to source pixels
        x_right_weight = (x_ratio * j) - x;
        y_bottom_weight = (y_ratio * i) - y;

        // Evaluate dest pixel value
        dest[offset++]
          = ul * (1-x_right_weight ) * (1-y_bottom_weight)
          + ur * (  x_right_weight ) * (1-y_bottom_weight)
          + ll * (  y_bottom_weight) * (1-x_right_weight )
          + lr * (  x_right_weight ) * (  y_bottom_weight);
      }

  return result;
}


z_image *scale_zimage(z_image *image, int dest_width, int dest_height)
{
  if ( (image == NULL) || (dest_width == 0) || (dest_height == 0) )
    return NULL;
  else
    return resizeBilinear(image, dest_width, dest_height);
}


z_image *scale_zimage_to_width(z_image *image, int dest_width)
{
  float scale_factor = (float)image->width / (float)dest_width;
  int dest_height = image->height / scale_factor;
  return scale_zimage(image, dest_width, dest_height);
}


z_image *zimage_dup(z_image *image)
{
  size_t len;
  z_image *result;

  if ((result = (z_image*)malloc(sizeof(z_image))) == NULL)
    return NULL;

  len
    = image->width * image->height
    * (image->image_type == DRILBO_IMAGE_TYPE_RGB ? 3 : 1);

  //printf("%ld\n", (long)len);

  result->bits_per_sample = image->bits_per_sample;
  result->width = image->width;
  result->height = image->height;
  result->image_type = image->image_type;
  if ((result->data = (uint8_t*)malloc(len)) == NULL)
  { free(result); return NULL; }

  memcpy(result->data, image->data, len);

  return result;
}


void free_zimage(z_image *image)
{
  free(image->data);
  free(image);
}


char *get_drilbo_version()
{
  return DRILBO_VERSION;
}

#endif /* drilbo_c_INCLUDED */

