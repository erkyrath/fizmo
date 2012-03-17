
/* streams.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2009-2012 Christoph Ender.
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


#ifndef streams_c_INCLUDED
#define streams_c_INCLUDED

#include <string.h>

#include "../screen_interface/screen_interface.h"
#include "../tools/types.h"
#include "../tools/tracelog.h"
#include "../tools/i18n.h"
#include "../tools/z_ucs.h"
#include "../tools/unused.h"
#include "../tools/filesys.h"
#include "streams.h"
#include "config.h"
#include "fizmo.h"
#include "wordwrap.h"
#include "text.h"
#include "zpu.h"
#include "output.h"
#include "../locales/libfizmo_locales.h"

#ifndef DISABLE_BLOCKBUFFER
#include "blockbuf.h"
#endif /* DISABLE_BLOCKBUFFER */

#ifndef DISABLE_OUTPUT_HISTORY
#include "history.h"
#endif /* DISABLE_OUTPUT_HISTORY */

//int stream_active[5] = { 0, 1, 0, 0, 0 };
bool stream_1_active = true;
bool stream_2_was_already_active = false;
// stream2 is stored in dynamic memory at z_mem[0x11].
// stream3 is active when stream_3_current_depth >= -1
bool stream_4_active = false;
/*@dependent@*/ /*@null@*/ static z_file *stream_2 = NULL;
/*@only@*/ char *stream_2_filename = NULL;
static size_t stream_2_filename_size = 0;
/*@only@*/ static WORDWRAP *stream_2_wrapper = NULL;
static zscii current_filename_buffer[MAXIMUM_SCRIPT_FILE_NAME_LENGTH+1];
z_ucs last_script_filename[MAXIMUM_SCRIPT_FILE_NAME_LENGTH + 1];
static bool script_wrapper_active = true;
static uint8_t *stream_3_start[MAXIMUM_STREAM_3_DEPTH];
static uint8_t *stream_3_index[MAXIMUM_STREAM_3_DEPTH];
static int stream_3_current_depth = -1;
z_file *stream_4 = NULL;
/*@only@*/ static char *stream_4_filename;
static size_t stream_4_filename_size = 0;
z_ucs last_stream_4_filename[MAXIMUM_SCRIPT_FILE_NAME_LENGTH + 1];
z_ucs last_input_stream_filename[MAXIMUM_SCRIPT_FILE_NAME_LENGTH + 1];
static bool stream_4_init_underway = false;
char *input_stream_1_filename = NULL;
size_t input_stream_1_filename_size = 0;
z_file *input_stream_1 = NULL;
static bool input_stream_init_underway = false;
bool input_stream_1_active = false;
bool input_stream_1_was_already_active = false;

// This flag is used for the timed input. Since the verification routine
// may print some text to the screen, the interpreter must be able to
// restore the input line once it resumes input.
bool stream_output_has_occured = false;
static bool stream_2_init_underway = false;
static bool stream_4_was_already_active = false;



void open_streams()
{
#ifdef ENABLE_TRACING
  turn_on_trace();
#endif /* ENABLE_TRACING */
}


/*@-paramuse@*/
static void stream_2_wrapped_output_destination(z_ucs *z_ucs_output,
    void *UNUSED(dummy))
{
  char buf[128];
  int len;

  // FIMXE: Re-implement for various output charsets.

  if (*z_ucs_output != 0)
  {
    while (*z_ucs_output != 0)
    {
      len = zucs_string_to_utf8_string(buf, &z_ucs_output, 128);
      fsi->writechars(buf, len-1, stream_2);
    }
    if (strcmp(get_configuration_value("sync-transcript"), "true") == 0)
      fsi->flushfile(stream_2);
  }
}
/*@+paramuse@*/


/*
static void stream_2_do_nothing(void *UNUSED(destination_parameter))
{
};


static struct wordwrap_target streams_wordwrap_target =
{
  &stream_2_wrapped_output_destination,
  &stream_2_do_nothing,
  &stream_2_do_nothing
};
*/


void init_streams()
{
  char *src;
  size_t bytes_required;
  char *stream_2_line_width = get_configuration_value("stream-2-line-width");
  char *stream_2_left_margin = get_configuration_value("stream-2-left-margin");
  int stream2width, stream2margin;

  stream2width
    = stream_2_line_width != NULL       
    ? atoi(stream_2_line_width)
    : DEFAULT_STREAM_2_LINE_WIDTH;

  stream2margin
    = stream_2_left_margin != NULL       
    ? atoi(stream_2_left_margin)
    : DEFAULT_STREAM_2_LEFT_PADDING;

  stream_2_wrapper = wordwrap_new_wrapper(
      stream2width,
      &stream_2_wrapped_output_destination,
      NULL,
      true,
      stream2margin,
      (strcmp(get_configuration_value("sync-transcript"), "true") == 0
       ? true
       : false),
      (strcmp(get_configuration_value("disable-stream-2-hyphenation"),"true")==0
       ? false
       : true));

  if (strcmp(get_configuration_value("start-script-when-story-starts"),
        "true") == 0)
    z_mem[0x11] |= 1;

  if (strcmp(get_configuration_value(
          "start-command-recording-when-story-starts"), "true") == 0)
    stream_4_active = true;

  if ((src = get_configuration_value("transcript-filename")) != NULL)
  {
    bytes_required = strlen(src) + 1;
    if ((stream_2_filename == NULL) || (bytes_required>stream_2_filename_size))
    {
      stream_2_filename
        = (char*)fizmo_realloc(stream_2_filename, bytes_required);
      stream_2_filename_size = bytes_required;
    }

    strcpy(stream_2_filename, src);
    stream_2_was_already_active = true;
  }
  else
    src = fizmo_strdup(DEFAULT_TRANSCRIPT_FILE_NAME);

  (void)latin1_string_to_zucs_string(
      last_script_filename,
      src,
      strlen(src) + 1);

  TRACE_LOG("Converted script default filename: '");
  TRACE_LOG_Z_UCS(last_script_filename);
  TRACE_LOG("'.\n");

  if ((src = get_configuration_value("input-command-filename")) != NULL)
  {
    bytes_required = strlen(src) + 1;
    if ((input_stream_1_filename == NULL)
        || (bytes_required > input_stream_1_filename_size))
    {
      input_stream_1_filename
        = (char*)fizmo_realloc(input_stream_1_filename, bytes_required);
      input_stream_1_filename_size = bytes_required;
    }

    strcpy(input_stream_1_filename, src);
    input_stream_1_was_already_active = true;
  }
  else
    src = fizmo_strdup(DEFAULT_INPUT_COMMAND_FILE_NAME);

  (void)latin1_string_to_zucs_string(
      last_input_stream_filename,
      src,
      strlen(src) + 1);

  if ((src = get_configuration_value("record-command-filename")) != NULL)
  {
    bytes_required = strlen(src) + 1;
    if ((stream_4_filename == NULL) || (bytes_required>stream_4_filename_size))
    {
      stream_4_filename
        = (char*)fizmo_realloc(stream_4_filename, bytes_required);
      stream_4_filename_size = bytes_required;
    }

    strcpy(stream_4_filename, src);
    stream_4_was_already_active = true;
  }
  else
    src = fizmo_strdup(DEFAULT_RECORD_COMMAND_FILE_NAME);

  (void)latin1_string_to_zucs_string(
      last_stream_4_filename,
      src,
      strlen(src) + 1);

  if (strcmp(get_configuration_value(
          "start-file-input-when-story-starts"), "true") == 0)
    input_stream_1_active = true;
}


void ask_for_input_stream_filename(void)
{
  z_ucs *current_line;
  bool stream_1_active_buf;
  int16_t input_length;
  int16_t i;
  size_t bytes_required;
  z_ucs *ptr;
  int len;

  input_stream_init_underway = true;

#ifndef DISABLE_OUTPUT_HISTORY
  current_line = get_current_line(outputhistory[active_window_number]);
#else
  current_line = NULL;
#endif /* DISABLE_OUTPUT_HISTORY */

  stream_1_active_buf = stream_1_active;
  stream_1_active = true;
  (void)i18n_translate(
      libfizmo_module_name,
      i18n_libfizmo_PLEASE_ENTER_NAME_FOR_COMMANDFILE);

  (void)streams_latin1_output("\n>");

  (void)streams_z_ucs_output(last_input_stream_filename);
  len = z_ucs_len(last_input_stream_filename);
#ifndef DISABLE_OUTPUT_HISTORY
  remove_chars_from_history(outputhistory[active_window_number], len);
#endif /* DISABLE_OUTPUT_HISTORY */

  for (i=0; i<len; i++)
  {
    current_filename_buffer[i]
      = unicode_char_to_zscii_input_char(last_input_stream_filename[i]);
  }

  do
  {
    input_length
      = active_interface->read_line(
          (zscii*)current_filename_buffer,
          MAXIMUM_SCRIPT_FILE_NAME_LENGTH,
          0,
          0,
          len,
          NULL,
          true,
          false);

    if (input_length == 0)
    {
      if (streams_latin1_output("\n") != 0)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "streams_latin1_output");

      if (i18n_translate(
            libfizmo_module_name,
            i18n_libfizmo_FILENAME_MUST_NOT_BE_EMPTY) == (size_t)-1)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "i18n_translate");

      if (streams_latin1_output("\n") != 0)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "streams_latin1_output");
    }
  }
  while (input_length == 0);

  for (i=0; i<input_length; i++)
    last_script_filename[i]
      = zscii_input_char_to_z_ucs(current_filename_buffer[i]);
  last_script_filename[i] = 0;

  stream_1_active = active_interface->input_must_be_repeated_by_story();
  (void)streams_z_ucs_output_user_input(last_script_filename);
  (void)streams_z_ucs_output_user_input(z_ucs_newline_string);

  stream_1_active = stream_1_active_buf;

  TRACE_LOG("From ZSCII translated filename: \"");
  TRACE_LOG_Z_UCS(last_script_filename);
  TRACE_LOG("\".\n");

  ptr = last_script_filename;
  bytes_required = (size_t)zucs_string_to_utf8_string(NULL, &ptr, 0);

  if (
      (input_stream_1_filename == NULL)
      ||
      (bytes_required > input_stream_1_filename_size)
     )
  {
    TRACE_LOG("(Re-)allocating %zd bytes.\n", bytes_required);

    input_stream_1_filename
      = (char*)fizmo_realloc(input_stream_1_filename, bytes_required);

    input_stream_1_filename_size = bytes_required;
  }

  ptr = last_script_filename;

  TRACE_LOG("From ZSCII translated filename: \"");
  TRACE_LOG_Z_UCS(ptr);
  TRACE_LOG("\".\n");

  // FIXME: Charsets may differ on operating systems.
  (void)zucs_string_to_utf8_string(
      input_stream_1_filename,
      &ptr,
      bytes_required);

  TRACE_LOG("Converted filename: '%s'.\n", stream_2_filename);

  if (current_line != NULL)
  {
    (void)streams_z_ucs_output(current_line);
    free(current_line);
  }

  input_stream_init_underway = false;
}


void ask_for_stream2_filename()
{
  z_ucs *current_line;
  bool stream_1_active_buf;
  int16_t input_length;
  int16_t i;
  size_t bytes_required;
  z_ucs *ptr;
  int len;

  stream_2_init_underway = true;

#ifndef DISABLE_OUTPUT_HISTORY
  current_line = get_current_line(outputhistory[active_window_number]);
#else
  current_line = NULL;
#endif /* DISABLE_OUTPUT_HISTORY */

  stream_1_active_buf = stream_1_active;
  stream_1_active = true;
  (void)i18n_translate(
      libfizmo_module_name,
      i18n_libfizmo_PLEASE_ENTER_SCRIPT_FILENAME);

  (void)streams_latin1_output("\n>");

  if (streams_z_ucs_output(last_script_filename) != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "streams_z_ucs_output");

  len = z_ucs_len(last_script_filename);
#ifndef DISABLE_OUTPUT_HISTORY
  remove_chars_from_history(outputhistory[active_window_number], len);
#endif /* DISABLE_OUTPUT_HISTORY */

  for (i=0; i<len; i++)
  {
    current_filename_buffer[i]
      = unicode_char_to_zscii_input_char(last_script_filename[i]);
  }

  do
  {
    input_length
      = active_interface->read_line(
          (zscii*)current_filename_buffer,
          MAXIMUM_SCRIPT_FILE_NAME_LENGTH,
          0,
          0,
          len,
          NULL,
          true,
          false);

    if (input_length == 0)
    {
      if (streams_latin1_output("\n") != 0)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "streams_latin1_output");

      if (i18n_translate(
            libfizmo_module_name,
            i18n_libfizmo_FILENAME_MUST_NOT_BE_EMPTY) == (size_t)-1)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "i18n_translate");

      if (streams_latin1_output("\n") != 0)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "streams_latin1_output");
    }
  }
  while (input_length == 0);

  for (i=0; i<input_length; i++)
    last_script_filename[i]
      = zscii_input_char_to_z_ucs(current_filename_buffer[i]);
  last_script_filename[i] = 0;

  stream_1_active = active_interface->input_must_be_repeated_by_story();
  (void)streams_z_ucs_output_user_input(last_script_filename);
  (void)streams_z_ucs_output_user_input(z_ucs_newline_string);

  stream_1_active = stream_1_active_buf;

  TRACE_LOG("From ZSCII translated filename: \"");
  TRACE_LOG_Z_UCS(last_script_filename);
  TRACE_LOG("\".\n");

  ptr = last_script_filename;
  bytes_required = (size_t)zucs_string_to_utf8_string(NULL, &ptr, 0);

  if (
      (stream_2_filename == NULL)
      ||
      (bytes_required > stream_2_filename_size)
     )
  {
    TRACE_LOG("(Re-)allocating %zd bytes.\n", bytes_required);

    stream_2_filename
      = (char*)fizmo_realloc(stream_2_filename, bytes_required);

    stream_2_filename_size = bytes_required;
  }

  ptr = last_script_filename;

  TRACE_LOG("From ZSCII translated filename: \"");
  TRACE_LOG_Z_UCS(ptr);
  TRACE_LOG("\".\n");

  // FIXME: Charsets may differ on operating systems.
  (void)zucs_string_to_utf8_string(
      stream_2_filename,
      &ptr,
      bytes_required);

  TRACE_LOG("Converted filename: '%s'.\n", stream_2_filename);

  if (current_line != NULL)
  {
    (void)streams_z_ucs_output(current_line);
    free(current_line);
  }
  stream_2_was_already_active = true;
  stream_2_init_underway = false;
}


static void stream_2_output(z_ucs *z_ucs_output)
{
  if (
      (active_interface == NULL)
      ||
      (active_window_number != 0)
     )
    return ;

  if (
      (stream_2_was_already_active == false)
      &&
      (stream_2_init_underway == false)
     )
  {
    ask_for_stream2_filename();
  }

  if (stream_2_init_underway == false)
  {
    if (stream_2 == NULL)
    {
      TRACE_LOG("Opening script-file '%s' for writing.\n", stream_2_filename);
      stream_2 = fsi->openfile(
          stream_2_filename, FILETYPE_TRANSCRIPT, FILEACCESS_APPEND);
      fsi->writechar('\n', stream_2);
      wordwrap_output_left_padding(stream_2_wrapper);
      fsi->writestring("---\n\n", stream_2);
      wordwrap_output_left_padding(stream_2_wrapper);
      if (strcmp(get_configuration_value("sync-transcript"), "true") == 0)
        fsi->flushfile(stream_2);
    }

    if (bool_equal(lower_window_buffering_active, true))
    {
      if (script_wrapper_active == false)
      {
        script_wrapper_active = true;
      }
      wordwrap_wrap_z_ucs(stream_2_wrapper, z_ucs_output);
    }
    else
    {
      if (bool_equal(script_wrapper_active, true))
      {
        wordwrap_flush_output(stream_2_wrapper);
        script_wrapper_active = false;
      }
      stream_2_wrapped_output_destination(z_ucs_output, NULL);
    }
  }
}


#ifndef DISABLE_OUTPUT_HISTORY
void ask_for_stream4_filename_if_required(bool dont_output_current_line)
#else
void ask_for_stream4_filename_if_required(bool UNUSED(dont_output_current_line))
#endif // DISABLE_OUTPUT_HISTORY
{
  z_ucs *current_line = NULL;
  bool stream_1_active_buf;
  int16_t input_length;
  int16_t i;
  size_t bytes_required;
  z_ucs *ptr;
  int len;

  if (
      (bool_equal(stream_4_active, true))
      &&
      (stream_4_was_already_active == false)
      &&
      (stream_4_init_underway == false)
     )
  {
    stream_4_init_underway = true;

#ifndef DISABLE_OUTPUT_HISTORY
    if (bool_equal(dont_output_current_line, false))
    {
      current_line = get_current_line(outputhistory[active_window_number]);
      (void)streams_latin1_output("\n");
    }
#endif /* DISABLE_OUTPUT_HISTORY */

    stream_1_active_buf = stream_1_active;
    stream_1_active = true;

    do
    {
      (void)streams_latin1_output("\n");

      (void)i18n_translate(
          libfizmo_module_name,
          i18n_libfizmo_PLEASE_ENTER_NAME_FOR_COMMANDFILE);

      (void)streams_latin1_output("\n>");

      (void)streams_z_ucs_output(last_stream_4_filename);

      len = z_ucs_len(last_stream_4_filename);
#ifndef DISABLE_OUTPUT_HISTORY
      remove_chars_from_history(outputhistory[active_window_number], len);
#endif /* DISABLE_OUTPUT_HISTORY */

      for (i=0; i<len; i++)
      {
        current_filename_buffer[i]
          = unicode_char_to_zscii_input_char(last_stream_4_filename[i]);
      }

      input_length
        = active_interface->read_line(
            (zscii*)current_filename_buffer,
            MAXIMUM_SCRIPT_FILE_NAME_LENGTH,
            0,
            0,
            len,
            NULL,
            true,
            false);

      if (input_length == 0)
      {
        if (streams_latin1_output("\n") != 0)
          i18n_translate_and_exit(
              libfizmo_module_name,
              i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
              -0x0100,
              "streams_latin1_output");

        if (i18n_translate(
              libfizmo_module_name,
              i18n_libfizmo_FILENAME_MUST_NOT_BE_EMPTY) == (size_t)-1)
          i18n_translate_and_exit(
              libfizmo_module_name,
              i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
              -0x0100,
              "i18n_translate");

        if (streams_latin1_output("\n") != 0)
          i18n_translate_and_exit(
              libfizmo_module_name,
              i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
              -0x0100,
              "streams_latin1_output");
      }
    }
    while (input_length == 0);

    stream_1_active = stream_1_active_buf;

    for (i=0; i<input_length; i++)
      last_stream_4_filename[i]
        = zscii_input_char_to_z_ucs(current_filename_buffer[i]);
    last_stream_4_filename[i] = 0;

    (void)streams_z_ucs_output(last_stream_4_filename);
    (void)streams_latin1_output("\n\n");

    TRACE_LOG("From ZSCII translated filename: \"");
    TRACE_LOG_Z_UCS(last_stream_4_filename);
    TRACE_LOG("\".\n");

    ptr = last_stream_4_filename;
    bytes_required = (size_t)zucs_string_to_utf8_string(NULL, &ptr, 0);

    if (
        (stream_4_filename == NULL)
        ||
        (bytes_required > stream_4_filename_size)
       )
    {
      TRACE_LOG("(Re-)allocating %zd bytes.\n", bytes_required);

      stream_4_filename
        = (char*)fizmo_realloc(stream_4_filename, bytes_required);

      stream_4_filename_size = bytes_required;
    }

    // FIXME: Charsets may differ on operating systems.
    ptr = last_stream_4_filename;
    (void)zucs_string_to_utf8_string(
        stream_4_filename,
        &ptr,
        bytes_required);

    if (current_line != NULL)
    {
      (void)streams_z_ucs_output(current_line);
      free(current_line);
    }
    stream_4_was_already_active = true;
    stream_4_init_underway = false;
  }
}


void stream_4_latin1_output(char *latin1_output)
{
  if (
      (active_interface == NULL)
      ||
      (active_window_number != 0)
     )
    return ;

  // We'll ask at this point for the filename, since asking directly when
  // the OUTPUT_STREAM opcode is processed might garble then screen output.
  // So we'll just wair until the user has finished with the input and ask
  // for the filename to save to once he's finished.
  ask_for_stream4_filename_if_required(false);

  if (stream_4_init_underway == false)
  {
    if (stream_4 == NULL)
    {
      TRACE_LOG("Opening script-file '%s' for writing.\n", stream_4_filename);
      stream_4 = fsi->openfile(stream_4_filename, FILETYPE_INPUTRECORD,
          FILEACCESS_APPEND);
    }

    fsi->writechars(latin1_output, strlen(latin1_output), stream_4);
  }
}


void stream_4_z_ucs_output(z_ucs *z_ucs_output)
{
  char buf[128];

  while (*z_ucs_output != 0)
  {
    zucs_string_to_utf8_string(buf, &z_ucs_output, 128);
    stream_4_latin1_output(buf);
  }
}


static void close_script_file()
{
  if (stream_2 != NULL)
  {
    TRACE_LOG("Closing script-file.\n");

    wordwrap_flush_output(stream_2_wrapper);
    fsi->writechar('\n', stream_2);
    (void)fsi->closefile(stream_2);
    stream_2 = NULL;
  }
}


static int _streams_z_ucs_output(z_ucs *z_ucs_output, bool is_user_input)
{
  int16_t conversion_result;
  zscii zscii_char;
  z_ucs *src;
  uint16_t len;
  z_ucs buf[128];
  z_ucs *converted_output, *ptr;
  int size;
  bool font_conversion_active
    = (
        (current_font == Z_FONT_CHARACTER_GRAPHICS)
        &&
        (strcmp(get_configuration_value("enable-font3-conversion"),"true") == 0)
      ) ? true : false;

  TRACE_LOG("Streams-output of \"");
  TRACE_LOG_Z_UCS(z_ucs_output);
  TRACE_LOG("\".\n");

  if (
      (stream_3_current_depth != -1)
      &&
      (bool_equal(is_user_input, false))
     )
  {
    src = z_ucs_output;

    while (*src != 0)
    {
      conversion_result
        = (int16_t)unicode_char_to_zscii_input_char(*src);

      if (conversion_result == 10)
        zscii_char = (zscii)13;
      else if (conversion_result == -1)
        zscii_char = (zscii)'?';
      else
        zscii_char = (zscii)conversion_result;

      *(stream_3_index[stream_3_current_depth]++) = zscii_char;
      TRACE_LOG("Writing ZSCII '%c' / %d to %ld.\n",
          zscii_char,
          zscii_char,
          (long int)(stream_3_index[stream_3_current_depth] - z_mem));

      src++;
    }

    len = load_word(stream_3_start[stream_3_current_depth]);
    TRACE_LOG("Loaded current stream-3-length %d.\n", len);

    len += (src - z_ucs_output);

    store_word(stream_3_start[stream_3_current_depth], len);
    TRACE_LOG("Stored current stream-3-length %d.\n", len);
  }
  else
  {
    if (bool_equal(is_user_input, false))
      stream_output_has_occured = true;

    if (
        (active_z_story != NULL)
        &&
        ((z_mem[0x11] & 0x1) != 0)
        &&
        (ver != 6)
        &&
        (strcasecmp(get_configuration_value("disable-external-streams"), "true")
         != 0)
       )
    {
      stream_2_output(z_ucs_output);
    }
    else if (stream_2 != NULL)
    {
      close_script_file();
    }

    converted_output = font_conversion_active == true ? buf : z_ucs_output;
    ptr = z_ucs_output;
    while (*ptr != 0)
    {
      if (font_conversion_active == true)
      {
        TRACE_LOG("Converting to font 3.\n");

        if ((size = z_ucs_len(ptr)) > 127)
          size = 127;
        memcpy(buf, ptr, size * sizeof(z_ucs));
        buf[size] = 0;
        ptr += size;
        size--;
        while (size >= 0)
        {
          TRACE_LOG("size:%d (%d).\n", size, buf[size]);

          if ( (buf[size] == 32) || (buf[size] == 37) ) // empty
            buf[size] = ' ';
          if (buf[size] == 33) // arrow left
            buf[size] = 0x2190;
          else if (buf[size] == 34) // arrow right
            buf[size] = 0x2192;
          else if (buf[size] == 35) // diagonal
            //buf[size] = 0x2571;
            buf[size] = '/';
          else if (buf[size] == 36) // diagonal
            //buf[size] = 0x2572;
            buf[size] = '\\';
          else if (buf[size] == 38) // horizontal
            buf[size] = 0x2500;
          else if (buf[size] == 39) // horizontal
            buf[size] = 0x2500;
          else if (buf[size] == 40)
            buf[size] = 0x2502;
          else if (buf[size] == 41)
            buf[size] = 0x2502;
          else if (buf[size] == 42)
            buf[size] = 0x2534;
          else if (buf[size] == 43)
            buf[size] = 0x252C;
          else if (buf[size] == 44)
            buf[size] = 0x251C;
          else if (buf[size] == 45)
            buf[size] = 0x2524;
          else if (buf[size] == 46)
            buf[size] = 0x230A;
          else if (buf[size] == 47)
            buf[size] = 0x2308;
          else if (buf[size] == 48)
            buf[size] = 0x2309;
          else if (buf[size] == 49)
            buf[size] = 0x230B;
          else if (buf[size] == 50) // FIXME: Better symbol
            buf[size] = 0x2534;
          else if (buf[size] == 51) // FIXME: Better symbol
            buf[size] = 0x252C;
          else if (buf[size] == 52) // FIXME: Better symbol
            buf[size] = 0x251C;
          else if (buf[size] == 53) // FIXME: Better symbol
            buf[size] = 0x2524;
          else if (buf[size] == 54) // full block
            buf[size] = 0x2588;
          else if (buf[size] == 55) // upper half block
            buf[size] = 0x2580;
          else if (buf[size] == 56) // lower half block
            buf[size] = 0x2584;
          else if (buf[size] == 57) // left half block
            buf[size] = 0x258C;
          else if (buf[size] == 58) // right half block
            buf[size] = 0x2590;
          else if (buf[size] == 59) // FIXME: Better symbol
            buf[size] = 0x2584;
          else if (buf[size] == 60) // FIXME: Better symbol
            buf[size] = 0x2580;
          else if (buf[size] == 61) // FIXME: Better symbol
            buf[size] = 0x258C;
          else if (buf[size] == 62) // FIXME: Better symbol
            buf[size] = 0x2590;
          else if (buf[size] == 63) // upper right block
            buf[size] = 0x259D;
          else if (buf[size] == 64) // lower right block
            buf[size] = 0x2597;
          else if (buf[size] == 65) // lower left block
            buf[size] = 0x2596;
          else if (buf[size] == 66) // upper left block
            buf[size] = 0x2598;
          else if (buf[size] == 67) // FIXME: Better symbol
            buf[size] = 0x259D;
          else if (buf[size] == 68) // FIXME: Better symbol
            buf[size] = 0x2597;
          else if (buf[size] == 69) // FIXME: Better symbol
            buf[size] = 0x2596;
          else if (buf[size] == 70) // FIXME: Better symbol
            buf[size] = 0x2598;
          else if (buf[size] == 71) // dot upper right, FIXME: Better symbol
            buf[size] = '+';
          else if (buf[size] == 72) // dot lower right, FIXME: Better symbol
            buf[size] = '+';
          else if (buf[size] == 73) // dot lower left, FIXME: Better symbol
            buf[size] = '+';
          else if (buf[size] == 74) // dot upper left, FIXME: Better symbol
            buf[size] = '+';
          else if (buf[size] == 75) // line top
            buf[size] = 0x2594;
          else if (buf[size] == 76) // line bottom
            buf[size] = '_';
          else if (buf[size] == 77) // line left
            buf[size] = 0x23B9;
          else if (buf[size] == 78) // line right
            buf[size] = 0x2595;
          else if (buf[size] == 79) // status bar, filled 0/8
            buf[size] = ' ';
          else if (buf[size] == 80) // status bar, filled 1/8
            buf[size] = 0x258F;
          else if (buf[size] == 81) // status bar, filled 2/8
            buf[size] = 0x258E;
          else if (buf[size] == 82) // status bar, filled 3/8
            buf[size] = 0x258D;
          else if (buf[size] == 83) // status bar, filled 4/8
            buf[size] = 0x258C;
          else if (buf[size] == 84) // status bar, filled 5/8
            buf[size] = 0x258B;
          else if (buf[size] == 85) // status bar, filled 6/8
            buf[size] = 0x258A;
          else if (buf[size] == 86) // status bar, filled 7/8
            buf[size] = 0x2589;
          else if (buf[size] == 87) // status bar, filled 8/8
            buf[size] = 0x2588;
          else if (buf[size] == 88) // status bar, padding right
            buf[size] = 0x2595;
          else if (buf[size] == 89) // status bar, padding left
            buf[size] = 0x258F;
          else if (buf[size] == 90) // diagonal cross
            buf[size] = 0x2573;
          else if (buf[size] == 91) // vertical / horizontal cross
            buf[size] = 0x253C;
          else if (buf[size] == 92) // arrow up
            buf[size] = 0x2191;
          else if (buf[size] == 93) // arrow down
            buf[size] = 0x2193;
          else if (buf[size] == 94) // arrow up-down
            buf[size] = 0x2195;
          else if (buf[size] == 95) // square
            buf[size] =0x25A2;
          else if (buf[size] == 96) // question mark
            buf[size] = '?';

          // runic

          else if (buf[size] == 97)
            buf[size] = 0x16aa;
          else if (buf[size] == 98)
            buf[size] = 0x16d2;
          else if (buf[size] == 99)
            buf[size] = 0x16c7;
          else if (buf[size] == 100)
            buf[size] = 0x16de;
          else if (buf[size] == 101)
            buf[size] = 0x16d6;
          else if (buf[size] == 102)
            buf[size] = 0x16a0;
          else if (buf[size] == 103)
            buf[size] = 0x16b7;
          else if (buf[size] == 104)
            buf[size] = 0x16bb;
          else if (buf[size] == 105)
            buf[size] = 0x16c1;
          else if (buf[size] == 106)
            buf[size] = 0x16e8;
          else if (buf[size] == 107)
            buf[size] = 0x16e6;
          else if (buf[size] == 108)
            buf[size] = 0x16da;
          else if (buf[size] == 109)
            buf[size] = 0x16d7;
          else if (buf[size] == 110)
            buf[size] = 0x16be;
          else if (buf[size] == 111)
            buf[size] = 0x16a9;
          else if (buf[size] == 112) // FIXME: better symbol?
            buf[size] = 0x16b3;
          else if (buf[size] == 113)
            buf[size] = 'h';         // FIXME: better symbol?;
          else if (buf[size] == 114)
            buf[size] = 0x16b1;
          else if (buf[size] == 115)
            buf[size] = 0x16cb;
          else if (buf[size] == 116)
            buf[size] = 0x16cf;
          else if (buf[size] == 117)
            buf[size] = 0x16a2;
          else if (buf[size] == 118)
            buf[size] = 0x16e0;
          else if (buf[size] == 119)
            buf[size] = 0x16b9;
          else if (buf[size] == 120)
            buf[size] = 0x16c9;
          else if (buf[size] == 121)
            buf[size] = 0x16a5;
          else if (buf[size] == 122)
            buf[size] = 0x16df;

          // inverted

          else if (buf[size] == 123) // inverted arrow up, FIXME: symbol
            buf[size] = 0x2191;
          else if (buf[size] == 124) // inverted arrow down, FIXME: symbol
            buf[size] = 0x2193;
          else if (buf[size] == 125) // inverted arrow up-down, FIXME: symbol
            buf[size] = 0x2195;
          else if (buf[size] == 126) // inverted question mark, FIXME: symbol
            buf[size] = '?';

          size--;
        }
      }

      if (bool_equal(stream_1_active, true))
      {
        if (active_interface != NULL)
        {
          active_interface->z_ucs_output(converted_output);
        }

#ifndef DISABLE_OUTPUT_HISTORY
        if (active_window_number == 0)
        {
          // outputhistory[0] is always defined if DISABLE_OUTPUT_HISTORY is not
          // defined (see fizmo.c).

          if (lower_window_style != current_style)
          {
            store_metadata_in_history(
                outputhistory[0],
                HISTORY_METADATA_TYPE_STYLE,
                current_style);
            lower_window_style = current_style;
          }

          if (
              (lower_window_foreground_colour != current_foreground_colour)
              ||
              (lower_window_background_colour != current_background_colour)
             )
          {
            store_metadata_in_history(
                outputhistory[0],
                HISTORY_METADATA_TYPE_COLOUR,
                current_foreground_colour,
                current_background_colour);

            lower_window_foreground_colour = current_foreground_colour;
            lower_window_background_colour = current_background_colour;
          }
        }

        if (
            (
             (ver == 6)
             ||
             (active_window_number != 1)
            )
            &&
            (outputhistory[active_window_number] != NULL)
           )
        {
          store_z_ucs_output_in_history(
              outputhistory[0],
              converted_output);
        }
#endif /* DISABLE_OUTPUT_HISTORY */

#ifndef DISABLE_BLOCKBUFFER
        if ((active_window_number == 1) && (upper_window_buffer != NULL))
        {
          if (upper_window_style != current_style)
          {
            set_blockbuf_style(upper_window_buffer, current_style);
            upper_window_style = current_style;
          }

          if (upper_window_foreground_colour != current_foreground_colour)
          {
            set_blockbuf_foreground_colour(
                upper_window_buffer, current_foreground_colour);
            upper_window_foreground_colour = current_foreground_colour;
          }

          if (upper_window_background_colour != current_background_colour)
          {
            set_blockbuf_background_colour(
                upper_window_buffer, current_background_colour);
            upper_window_background_colour = current_background_colour;
          }

          store_z_ucs_output_in_blockbuffer(
              upper_window_buffer, converted_output);
        }
#endif /* DISABLE_BLOCKBUFFER */
      }

      if (font_conversion_active == false)
        break;
    }

    if (bool_equal(is_user_input, true))
    {
      if (
          (bool_equal(stream_4_active, true))
          &&
          (strcasecmp(get_configuration_value("disable-external-streams"),
                      "true") != 0)
         )
        stream_4_z_ucs_output(z_ucs_output);
      else
      {
        if (stream_4 != NULL)
        {
          (void)fsi->closefile(stream_4);
          stream_4 = NULL;
	}
      }
    }
  }

  /*@-globstate@*/
  return 0;
  /*@+globstate@*/

  // For some reason splint thinks that comparison with NULL might set
  // active_z_story or active_interface to NULL. The "globstate" annotations
  // above inhibit these warnings.
}


int streams_z_ucs_output(z_ucs *z_ucs_output)
{
  return _streams_z_ucs_output(z_ucs_output, false);
}


int streams_z_ucs_output_user_input(z_ucs *z_ucs_output)
{
  return _streams_z_ucs_output(z_ucs_output, true);
}


int streams_latin1_output(/*@in@*/ char *latin1_output)
{
  z_ucs output_buffer_z_ucs[ASCII_TO_Z_UCS_BUFFER_SIZE];

  //FIXME: latin1 != utf8
  while (latin1_output != NULL)
  {
    latin1_output = utf8_string_to_zucs_string(
        output_buffer_z_ucs,
        latin1_output,
        ASCII_TO_Z_UCS_BUFFER_SIZE);

    if (streams_z_ucs_output(output_buffer_z_ucs) != 0)
      return -1;
  }

  return 0;
}


void opcode_output_stream(void)
{
  int16_t stream_number =(int16_t) op[0];

  TRACE_LOG("Opcode: OUTPUT_STREAM.\n");

  if (stream_number == 0)
    return;

#ifdef ENABLE_TRACING
  if (((int16_t)op[0]) < 0)
  {
    TRACE_LOG("Closing stream ");
  }
  else
  {
    TRACE_LOG("Opening stream ");
  }
  TRACE_LOG("%d.\n", (((int16_t)op[0]) < 0 ? -((int16_t)op[0]) : op[0]));
#endif

  if ((stream_number < -4) || (stream_number > 4))
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_INVALID_OUTPUT_STREAM_NUMBER_P0D,
        -1,
        (long int)stream_number);

  if (stream_number == 1)
    stream_1_active = true;

  else if (stream_number == -1)
    stream_1_active = false;

  else if (stream_number == 2)
    z_mem[0x11] |= 1;

  else if (stream_number == -2)
    z_mem[0x11] &= 0xfe;

  else if (stream_number == 3)
  {
    if (++stream_3_current_depth == MAXIMUM_STREAM_3_DEPTH)
      i18n_translate_and_exit(
          libfizmo_module_name,
          i18n_libfizmo_MAXIMUM_STREAM_3_DEPTH_P0D_EXCEEDED,
          -1,
          MAXIMUM_STREAM_3_DEPTH);

    stream_3_start[stream_3_current_depth] = z_mem + op[1];
    stream_3_index[stream_3_current_depth] = z_mem + op[1] + 2;

    store_word(stream_3_start[stream_3_current_depth], 0);

    TRACE_LOG("stream-3 depth: %d.\n", stream_3_current_depth);

    TRACE_LOG("Current stream-3 dest is %p.\n", 
        stream_3_start[stream_3_current_depth]);
  }

  else if (stream_number == -3)
  {
    if (stream_3_current_depth >= 0)
    {
      TRACE_LOG("stream-3 depth: %d.\n", stream_3_current_depth);
      stream_3_current_depth--;
    }
  }

  else if (stream_number == 4)
    stream_4_active = true;

  else if (stream_number == -4)
    stream_4_active = false;
}


void close_streams(z_ucs *error_message)
{
  TRACE_LOG("Closing all streams.\n");

  // Close the interface first. This will allow stream 2 to capture
  // any pending output.
  (void)close_interface(error_message);

  if (stream_2 != NULL)
    close_script_file();

  if (stream_4 != NULL)
  {
    (void)fsi->closefile(stream_4);
    stream_4 = NULL;
  }

#ifdef ENABLE_TRACING
  turn_off_trace();
#endif /* ENABLE_TRACING */
}


void open_input_stream_1(void)
{
  if (strcasecmp(get_configuration_value("disable-external-streams"), "true")
      != 0)
  {
    input_stream_1_active = true;
  }
  else
  {
    (void)i18n_translate(
        libfizmo_module_name, i18n_libfizmo_THIS_FUNCTION_HAS_BEEN_DISABLED);
    (void)streams_latin1_output("\n");
  }
}


void close_input_stream_1(void)
{
  if (input_stream_1 != NULL)
  {
    fsi->closefile(input_stream_1);
    input_stream_1 = NULL;
  }
}


void opcode_input_stream(void)
{
  TRACE_LOG("Opcode: INPUT_STREAM.\n");

  if (((int16_t)op[0]) == 1)
  {
    open_input_stream_1();
  }
  else
  {
    close_input_stream_1();
  }
}

#endif // streams_c_INCLUDED

