
/* iff.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2009-2011 Christoph Ender.
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


// iff.c provides a simple wrapper for managing IFF files. "Simple" meaning
// that it has been tailored around the needs for the quetzal save file
// format and thus will almost certainly not work with other iff related
// data.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "../tools/tracelog.h"
#include "../tools/i18n.h"
#include "../tools/types.h"
#include "fizmo.h"
#include "iff.h"
#include "../locales/libfizmo_locales.h"


static int current_mode;
//static int current_form_length;
static long current_chunk_offset;
static char four_chars[5];
static int last_chunk_length;
//static int filename_utf8_size = 0;


static int read_four_chars(void *iff_file)
{
  int data;
  int i;

  int (*getchar)(void *) = active_filesys_interface->getchar;

  four_chars[4] = '\0';
  for (i=0; i<4; i++)
  {
    data = getchar(iff_file);
    if (data == EOF)
      return -1;
    four_chars[i] = (char)data;
  }

  return 0;
}


/*@null@*/ /*@dependent@*/ void *open_simple_iff_file(char *filename, int mode)
{
  void *iff_file;

  if (filename == NULL)
    return NULL;

  TRACE_LOG("Trying to open IFF file \"%s\".\n", filename);

  current_mode = mode;

  if (mode == IFF_MODE_READ)
  {
    if ((iff_file = fopen(filename, "r")) == NULL)
      return NULL;

    if (read_four_chars(iff_file) != 0)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }

    if (strcmp(four_chars, "FORM") != 0)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }

    if (read_chunk_length(iff_file) != 0)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }

    //current_form_length = last_chunk_length;

    //TRACE_LOG("IFF FORM chunk length: %d.\n", current_form_length);

    if (read_four_chars(iff_file) != 0)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }

    /*
    if (strcmp(four_chars, "IFZS") != 0)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }
    */
  }
  else if (mode == IFF_MODE_WRITE)
  {
    if ((iff_file = fopen(filename, "w")) == NULL)
      return NULL;

    if ((active_filesys_interface->putchars)("FORM\0\0\0\0IFZS", 12, iff_file) < 12)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return NULL;
    }
  }
  else
  {
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_INVALID_IFF_ACCESS_MODE_P0D,
        -1,
        (long int)mode);
    /*@-unreachable@*/
    return NULL; // Will never be executed, just to put compiler at ease.
    /*@+unreachable@*/
  }

  return iff_file;
}


int get_last_chunk_length()
{
  return last_chunk_length;
}


int read_chunk_length(void *iff_file)
{
  last_chunk_length = read_four_byte_number(iff_file);
  TRACE_LOG("last_chunk_length set to %d.\n", last_chunk_length);
  return 0;
}


int start_new_chunk(char *id, void *iff_file)
{
  if ((active_filesys_interface->putchars)(id, 4, iff_file) < 4)
    return -1;

  if ((active_filesys_interface->putchars)("\0\0\0\0", 4, iff_file) < 4)
    return -1;

  if ((current_chunk_offset = (active_filesys_interface->getfilepos)(iff_file)) == -1)
    return -1;

  return 0;
}


int write_four_byte_number(uint32_t number, void *iff_file)
{
  int (*putchar)(int, void *) = active_filesys_interface->putchar;

  if (putchar((int)(number >> 24), iff_file) == EOF)
    return -1;

  if (putchar((int)(number >> 16), iff_file) == EOF)
    return -1;

  if (putchar((int)(number >>  8), iff_file) == EOF)
    return -1;

  if (putchar((int)(number      ), iff_file) == EOF)
    return -1;

  return 0;
}


int end_current_chunk(void *iff_file)
{
  long current_offset;
  long chunk_length;
  uint32_t chunk_length_uint32_t;

  if ((current_offset = (active_filesys_interface->getfilepos)(iff_file)) == -1)
    return -1;

  chunk_length = current_offset - current_chunk_offset;
  TRACE_LOG("Chunk length: %ld.\n", chunk_length);

  if ((uint32_t)chunk_length > UINT32_MAX)
    return -1;

  chunk_length_uint32_t = (uint32_t)chunk_length;

  if ((chunk_length_uint32_t & 1) != 0)
  {
    // 8.4.1: If length is odd, these are followed by a single zero
    // byte. This byte is *not* included in the chunk length ...
      if ((active_filesys_interface->putchar)(0, iff_file) == EOF)
      return -1;

    TRACE_LOG("Padding with single zero byte.\n");
  }

  TRACE_LOG("Seeking position %ld.\n", current_chunk_offset - 4);
  if ((active_filesys_interface->setfilepos)(iff_file, current_chunk_offset - 4, SEEK_SET) == -1)
    return -1;

  TRACE_LOG("Writing chunk length %d.\n", chunk_length_uint32_t);
  if (write_four_byte_number(chunk_length_uint32_t, iff_file) == -1)
    return -1;

  TRACE_LOG("Seeking end of file.\n");
  if ((active_filesys_interface->setfilepos)(iff_file, 0, SEEK_END) != 0)
    return -1;

  TRACE_LOG("Chunk writing finished.\n");

  return 0;
}


int close_simple_iff_file(void *iff_file)
{
  long length;
  uint32_t length_uint32_t;

  if ((active_filesys_interface->setfilepos)(iff_file, 0, SEEK_END) == -1)
    return -1;

  if ((length = (active_filesys_interface->getfilepos)(iff_file)) == -1)
    return -1;

  if ((uint32_t)length > UINT32_MAX)
    return -1;

  length -= 8;

  length_uint32_t = (uint32_t)length;

  if ((active_filesys_interface->setfilepos)(iff_file, 4, SEEK_SET) == -1)
    return -1;

  if (write_four_byte_number(length_uint32_t, iff_file) == -1)
    return -1;

  if ((active_filesys_interface->closefile)(iff_file) == EOF)
    return -1;

  return 0;
}


int find_chunk(char *id, void *iff_file)
{
  int chunk_length;

  TRACE_LOG("Looking for chunk \"%s\".\n", id);

  if ((active_filesys_interface->setfilepos)(iff_file, 12L, SEEK_SET) == -1)
  {
    TRACE_LOG("%s\n", strerror(errno));
    return -1;
  }

  for (;;)
  {
    if (read_four_chars(iff_file) != 0)
    {
      TRACE_LOG("%s\n", strerror(errno));
      return -1;
    }

    if (strcmp(id, four_chars) == 0)
    {
      TRACE_LOG("Found chunk \"%s\".\n", four_chars);
      return 0;
    }
    else
    {
      TRACE_LOG("Skipping chunk \"%s\".\n", four_chars);
    }

    if (read_chunk_length(iff_file) != 0)
      return -1;

    chunk_length = last_chunk_length;

    if ((chunk_length & 1) != 0)
      chunk_length++;

    TRACE_LOG("Skipping %d bytes.\n", chunk_length);
    if ((active_filesys_interface->setfilepos)(iff_file, chunk_length, SEEK_CUR) == -1)
    {
      TRACE_LOG("%s\n", strerror(errno));
      return -1;
    }
  }
}


uint32_t read_four_byte_number(void *iff_file)
{
  int data;
  int i;
  uint32_t result = 0;

  int (*getchar)(void *) = active_filesys_interface->getchar;

  for (i=0; i<4; i++)
  {
    data = getchar(iff_file);
    if (data == EOF)
    {
      (void)(active_filesys_interface->closefile)(iff_file);
      return -1;
    }
    result |= ((uint8_t)data)
      << /*@-shiftnegative@*/ 8*(3-i) /*@-shiftnegative@*/ ;
  }

  return result;
}


char *read_form_type(void *iff_file)
{
  if ((active_filesys_interface->setfilepos)(iff_file, 8L, SEEK_SET) == -1)
  {
    TRACE_LOG("%s\n", strerror(errno));
    return NULL;
  }

  if (read_four_chars(iff_file) != 0)
  {
    TRACE_LOG("%s\n", strerror(errno));
    return NULL;
  }

  return four_chars;
}


bool is_form_type(void *iff_file, char* form_type)
{
  return strcmp(read_form_type(iff_file), form_type) == 0 ? true : false;
}

