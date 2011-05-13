
/* savegame.c
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


#ifndef savegame_c_INCLUDED
#define savegame_c_INCLUDED

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "../tools/tracelog.h"
#include "../tools/i18n.h"
#include "../tools/z_ucs.h"
#include "savegame.h"
#include "streams.h"
#include "fizmo.h"
#include "iff.h"
#include "stack.h"
#include "zpu.h"
#include "variable.h"
#include "text.h"
#include "routine.h"
#include "filelist.h"
#include "history.h"
#include "output.h"
#include "config.h"
#include "../locales/libfizmo_locales.h"

#define HISTORY_BUFFER_INPUT_SIZE 1024


z_ucs last_savegame_filename[MAXIMUM_SAVEGAME_NAME_LENGTH + 1];
static zscii current_savegame_filename_buffer[MAXIMUM_SAVEGAME_NAME_LENGTH + 1];
//static z_ucs savegame_output_buffer[MAXIMUM_SAVEGAME_NAME_LENGTH + 1];


static int save_stack_frame(uint16_t *current_frame_index,
    uint16_t current_frame_stack_usage, uint8_t current_frame_number_of_locals,
    FILE *out_file)
{
  uint8_t previous_result_var;
  uint32_t previous_pc;
  uint16_t previous_stack_words_used;
  uint8_t previous_number_of_locals;
  bool previous_result_discard;
  uint8_t previous_argument_mask;
  uint8_t flags;
  uint16_t *data_index = current_frame_index;
  uint8_t i;

  TRACE_LOG("Saving stack frame.\n");
  TRACE_LOG("Z-Stack at %p.\n", z_stack);
  TRACE_LOG("Z-Stack-Index: %ld.\n", (long int)(current_frame_index - z_stack));
  TRACE_LOG("Data-Index: %ld.\n", (long int)(data_index - z_stack));
  TRACE_LOG("Current frame index: %p.\n", current_frame_index);
  TRACE_LOG("Current frame stack usage: %d.\n", current_frame_stack_usage);
  TRACE_LOG("Current frame number of locals: %d.\n",
      current_frame_number_of_locals);

  current_frame_index--;

  previous_result_var = (*current_frame_index & 0xff);

  previous_pc = (*current_frame_index >> 8);

  current_frame_index--;

  previous_pc |= ((*current_frame_index) << 8);

  current_frame_index--;

  previous_stack_words_used = *current_frame_index;

  current_frame_index--;

  previous_number_of_locals
    = (*current_frame_index & 0xf);

  previous_result_discard
    = ((*(current_frame_index) & 0x10) != 0 ? true : false);

  previous_argument_mask
    = (*(current_frame_index) >> 8);

  TRACE_LOG("Previous result var: %d.\n", previous_result_var);
  TRACE_LOG("Previous PC: %x.\n", previous_pc);
  TRACE_LOG("Previous stack words used: %d.\n", previous_stack_words_used);
  TRACE_LOG("Previous number of locales: %d.\n", previous_number_of_locals);

  // Save lower stack level first so serialized stack on disk starts
  // with index 0.
  if (current_frame_index != z_stack)
  {
    if (save_stack_frame(
          current_frame_index
          - previous_stack_words_used
          - previous_number_of_locals,
          previous_stack_words_used,
          previous_number_of_locals,
          out_file)
        != 0)
      return -1;
  }

  flags
    = (bool_equal(previous_result_discard, true) ? 0x10 : 0)
    | current_frame_number_of_locals;

  TRACE_LOG("Flags: %x.\n", flags);

  if (fputc((int)(previous_pc >> 16), out_file) == EOF)
    return -1;

  if (fputc((int)(previous_pc >>  8), out_file) == EOF)
    return -1;

  if (fputc((int)(previous_pc      ), out_file) == EOF)
    return -1;

  if (fputc((int)flags, out_file) == EOF)
    return -1;

  if (fputc((int)previous_result_var, out_file) == EOF)
    return -1;

  if (fputc((int)previous_argument_mask, out_file) == EOF)
    return -1;

  if (fputc((int)(current_frame_stack_usage >> 8), out_file) == EOF)
    return -1;

  if (fputc((int)(current_frame_stack_usage & 0xff), out_file) == EOF)
    return -1;

  TRACE_LOG("Data: (");
  for (i=0; i<current_frame_number_of_locals + current_frame_stack_usage; i++)
  {
    if (i != 0)
    {
      TRACE_LOG(", ");
    }
    TRACE_LOG("$%x", data_index[i]);

    if (fputc((int)(data_index[i] >> 8), out_file) == EOF)
      return -1;

    if (fputc((int)(data_index[i]     ), out_file) == EOF)
      return -1;
  }
  TRACE_LOG(")\n");

  TRACE_LOG("Final stack index: %ld.\n", (long int)(data_index - z_stack));

  return 0;
}


static int ask_for_filename(char *filename_suggestion)
{
  int16_t input_length;
  int length = 0;
  bool stream_1_active_buf;
  int i;

  TRACE_LOG("last:\"");
  TRACE_LOG_Z_UCS(last_savegame_filename);
  TRACE_LOG("\"\n");

  if (filename_suggestion != NULL)
  {
    TRACE_LOG("suggestion: '%s'.\n", filename_suggestion);
  }

  if (streams_latin1_output("\n") != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "streams_latin1_output");

  if (filename_suggestion != NULL)
  {
    TRACE_LOG("filename suggestion: %s.\n", filename_suggestion);

    length = strlen(filename_suggestion);
    i = 0;
    while ( (i < length) && (i + 1 < MAXIMUM_SAVEGAME_NAME_LENGTH) )
    {
      current_savegame_filename_buffer[i]
        = latin1_char_to_zucs_char(filename_suggestion[i]);

      /*
      savegame_output_buffer[i] =
        latin1_char_to_zucs_char(filename_suggestion[i]);
      */

      i++;
    }
    current_savegame_filename_buffer[i] = 0;
    //savegame_output_buffer[i] = 0;
    length = i;
  }
  else
  {
    TRACE_LOG("No filename suggestion.\n");
    length = z_ucs_len(last_savegame_filename);
    i = 0;
    while ( (i < length) && (i + 1 < MAXIMUM_SAVEGAME_NAME_LENGTH) )
    {
      current_savegame_filename_buffer[i]
        = unicode_char_to_zscii_input_char(last_savegame_filename[i]);

      /*
      savegame_output_buffer[i]
        = last_savegame_filename[i];
      */

      i++;
    }
    current_savegame_filename_buffer[i] = 0;
    /*
    savegame_output_buffer[i] = 0;
    length = i;
    TRACE_LOG("conv:");
    TRACE_LOG_Z_UCS(savegame_output_buffer);
    TRACE_LOG("\n");
    */
  }

  if (i18n_translate(
        libfizmo_module_name,
        i18n_libfizmo_PLEASE_ENTER_SAVEGAME_FILENAME) == (size_t)-1)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "i18n_translate");

  if (streams_latin1_output("\n>") != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
      i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
      -0x0100,
      "streams_latin1_output");

  if (streams_latin1_output((char*)current_savegame_filename_buffer) != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "streams_latin1_output");

  TRACE_LOG("Removing %d chars from history.\n", length);
#ifndef DISABLE_COMMAND_HISTORY
  remove_chars_from_history(outputhistory[active_window_number], length);
#endif /* DISABLE_COMMAND_HISTORY */

  // Prompt for filename
  input_length
    = active_interface->read_line(
        (uint8_t*)current_savegame_filename_buffer,
        MAXIMUM_SAVEGAME_NAME_LENGTH,
        0,
        0,
        length,
        NULL,
        true,
        true);

  if (input_length == -2)
  {
    // User pressed ESC

    if (streams_latin1_output("\n") != 0)
      i18n_translate_and_exit(
          libfizmo_module_name,
          i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
          -0x0100,
          "streams_latin1_output");

    return -2;
  }
  else if (input_length == 0)
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

    return -1;
  }

  current_savegame_filename_buffer[input_length] = 0;

  i = 0;
  while (i <= (int)input_length)
  {
    last_savegame_filename[i]
      = zscii_output_char_to_z_ucs(current_savegame_filename_buffer[i]);
    i++;
  }

  last_savegame_filename[i] = 0;

  TRACE_LOG("From ZSCII translated filename: \"");
  TRACE_LOG_Z_UCS(last_savegame_filename);
  TRACE_LOG("\".\n");

  stream_1_active_buf = stream_1_active;
  stream_1_active = active_interface->input_must_be_repeated_by_story();
  (void)streams_z_ucs_output_user_input(last_savegame_filename);
  (void)streams_z_ucs_output_user_input(z_ucs_newline_string);
  stream_1_active = stream_1_active_buf;

  if (streams_latin1_output("\n") != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "streams_latin1_output");

  return 0;
}


// Returns the supplied result code.
static int _store_save_or_restore_result(uint16_t result_code)
{
  if (ver < 4)
  {
    evaluate_branch(result_code != 0 ? (uint8_t)1 : (uint8_t)0);
  }
  else
  {
    read_z_result_variable();
    set_variable(z_res_var, result_code);
  }

  return result_code;
}


// Returns 0 for failure,  1 for "save succeeded" and 2 for "the game is
// being restored and is resuming execution again from here, the point where
// it was saved".
static int _handle_save_or_restore_failure(bool evaluate_result,
    int i18n_message_code, FILE *iff_file)
{
  if (i18n_message_code >= 1)
    if (i18n_translate(libfizmo_module_name, i18n_message_code) == (size_t)-1)
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

  if (fclose(iff_file) != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "fclose");

  if (bool_equal(evaluate_result, true))
    _store_save_or_restore_result(0);

  return 0;
}


int get_paragraph_save_amount()
{
  return strtol(
      get_configuration_value("save-text-history-paragraphs"), NULL, 10);
}


void save_game(uint16_t address, uint16_t length, char *filename,
    bool skip_asking_for_filename, bool evaluate_result, char *directory)
{
  uint32_t pc_on_restore = (uint32_t)(pc - z_mem);
  FILE *save_file;
  uint8_t *dynamic_index;
  uint16_t consecutive_zeros;
  int data;
  char *system_filename;
  uint8_t *ptr;
  int data2; // removeme
  char *str;
#ifndef DISABLE_OUTPUT_HISTORY
  z_ucs *hst_ptr;
  int nof_paragraphs_to_save;
  history_output *history;
  int return_code;
#endif // DISABLE_OUTPUT_HISTORY

  TRACE_LOG("Save %d bytes from address %d.\n", length, address);
  TRACE_LOG("PC at: %x.\n", pc_on_restore);

  if (filename != NULL)
  {
    if (bool_equal(skip_asking_for_filename, true))
    {
      system_filename = filename;
    }
    else
    {
      if ((ask_for_filename(filename)) < 0)
      {
        if (bool_equal(evaluate_result, true))
          _store_save_or_restore_result(0);
        return;
      }

      system_filename = dup_zucs_string_to_utf8_string(last_savegame_filename);
    }
  }
  else
  {
    if ((ask_for_filename(NULL)) < 0)
    {
      if (bool_equal(evaluate_result, true))
        _store_save_or_restore_result(0);
      return;
    }
    system_filename = dup_zucs_string_to_utf8_string(last_savegame_filename);
  }

  if (directory != NULL)
  {
    str = fizmo_malloc(strlen(system_filename) + strlen(directory) + 2);
    strcpy(str, directory);
    strcat(str, "/");
    strcat(str, system_filename);
  }
  else
    str = system_filename;

  TRACE_LOG("Filename to save to: \"%s\".\n", str);

  if (address != 0)
    save_file = fopen(str, "w");
  else
    save_file = open_simple_iff_file(str, IFF_MODE_WRITE);

  if (save_file == NULL)
  {
    if (i18n_translate(
          libfizmo_module_name,
          i18n_libfizmo_COULD_NOT_OPEN_FILE_NAMED_P0S, str)
        == (size_t)-1)
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

    if (directory != NULL)
      free(str);
    free(system_filename);

    if (bool_equal(evaluate_result, true))
      _store_save_or_restore_result(0);

    return;
  }

  if (address != 0)
  {
    if ((fwrite(z_mem + address, 1, length, save_file)) != length)
    {
      _handle_save_or_restore_failure(
          evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

    if ((fclose(save_file)) != 0)
    {
      _handle_save_or_restore_failure(
          evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }
  }
  else
  {
    if (directory != NULL)
      free(str);
    free(system_filename);

    if (start_new_chunk("IFhd", save_file) != 0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    // Save release number
    if ((fputc((int)z_mem[0x2], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x3], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    // Save serial number
    if ((fputc((int)z_mem[0x12], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x13], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x14], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x15], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x16], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x17], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    // Save checksum
    if ((fputc((int)z_mem[0x1c], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)z_mem[0x1d], save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    // Save initial PC on restore
    if ((fputc((int)(pc_on_restore >> 16), save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)(pc_on_restore >>  8), save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }
    if ((fputc((int)(pc_on_restore      ), save_file)) == EOF)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    if (end_current_chunk(save_file) != 0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
      return;
    }

    dynamic_index = z_mem + address;

    if (
        (active_z_story->z_file != NULL)
        &&
        ((fseek(
                active_z_story->z_file,
                active_z_story->story_file_exec_offset,
                SEEK_SET)) == 0)
        &&
        (strcmp(get_configuration_value("quetzal-umem"), "true") != 0)
       )
    {
      // The original story file is availiable, use CMem.
      TRACE_LOG("Compressing memory from byte %d ...\n", address);

      if (start_new_chunk("CMem", save_file) != 0)
      {
        _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
        return;
      }

      ptr = z_mem + address + length;
      consecutive_zeros = 0;

      while (dynamic_index != ptr)
      {
        if ((data = fgetc(active_z_story->z_file)) == EOF)
        {
          _handle_save_or_restore_failure(evaluate_result,
              i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
          return;
        }

        data2 = data; // removeme
        data ^= *dynamic_index;

        if (data == 0)
        {
          consecutive_zeros++;
        }
        else
        {
          TRACE_LOG("Altered byte at offset %ld.\n",
              (long int)(dynamic_index - z_mem));

          TRACE_LOG("Memory-Data: %x, Story-Data: %x.\n",
              *dynamic_index, data2);

          //TRACE_LOG("Skipping %d equal bytes.\n", consecutive_zeros);
          while (consecutive_zeros != 0)
          {
            if (fputc(0, save_file) == EOF)
            {
              _handle_save_or_restore_failure(evaluate_result,
                  i18n_libfizmo_ERROR_WRITING_SAVE_FILE, save_file);
              return;
            }

            if (consecutive_zeros > 256)
            {
              if (fputc(0xff, save_file) == EOF)
              {
                _handle_save_or_restore_failure(evaluate_result,
                    i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
                    save_file);
                return;
              }

              consecutive_zeros -= 256;
            }
            else
            {
              if (fputc((int)(consecutive_zeros - 1), save_file) == EOF)
              {
                _handle_save_or_restore_failure(evaluate_result,
                    i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
                    save_file);
                return;
              }

              consecutive_zeros = 0;
            }
          }

          if (fputc(data, save_file) == EOF)
          {
            _handle_save_or_restore_failure(evaluate_result,
                i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
                save_file);
            return;
          }
        }

        dynamic_index++;
      }
      TRACE_LOG("... to byte %ld.\n", (long int)(ptr - z_mem));

      if (end_current_chunk(save_file) != 0)
      {
        _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
            save_file);
        return;
      }
    }
    else
    {
      if (start_new_chunk("UMem", save_file) != 0)
        i18n_translate_and_exit(
            libfizmo_module_name,
            i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
            -0x0100,
            "start_new_chunk");

      while (dynamic_index != active_z_story->static_memory)
      {
        if ((fputc((int)(*dynamic_index), save_file)) == EOF)
        {
          _handle_save_or_restore_failure(
              evaluate_result,
              i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
              save_file);
          return;
        }
        dynamic_index++;
      }

      if (end_current_chunk(save_file) != 0)
      {
        _handle_save_or_restore_failure(
            evaluate_result,
            i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
            save_file);
        return;
      }
    }

    if (start_new_chunk("Stks", save_file) != 0)
    {
      _handle_save_or_restore_failure(
          evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

#ifdef ENABLE_TRACING
    dump_stack_to_tracelog();
    dump_dynamic_memory_to_tracelog();
#endif // ENABLE_TRACING

    // Save stack frames
    if (save_stack_frame(
          z_stack_index-stack_words_from_active_routine-number_of_locals_active,
          (uint16_t)stack_words_from_active_routine,
          number_of_locals_active,
          save_file) != 0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

    if (end_current_chunk(save_file) != 0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

    if (start_new_chunk("ANNO", save_file) != 0)
    {
      _handle_save_or_restore_failure(
          evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

    fprintf(save_file, "Interpreter: fizmo, version: %s.\n", FIZMO_VERSION);

    if (end_current_chunk(save_file) != 0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }

    nof_paragraphs_to_save = get_paragraph_save_amount();

#ifndef DISABLE_OUTPUT_HISTORY
    if (
        (nof_paragraphs_to_save > 0)
        &&
        (outputhistory[0] != NULL)
        &&
        (outputhistory[0]->z_history_buffer_size > 0)
       )
    {
      history = init_history_output(outputhistory[0], NULL);

      do
      {
        return_code = output_rewind_paragraph(history);
        nof_paragraphs_to_save--;
      }
      while ( (nof_paragraphs_to_save > 0) && (return_code == 0) );

      if (start_new_chunk("TxHs", save_file) != 0)
      {
        _handle_save_or_restore_failure(
            evaluate_result,
            i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
            save_file);
        return;
      }

      hst_ptr = history->current_paragraph_index;

      if (hst_ptr < outputhistory[0]->z_history_buffer_back_index)
      {
        while (hst_ptr != outputhistory[0]->z_history_buffer_end)
        {
          if (write_four_byte_number(*hst_ptr, save_file) != 0)
          {
            _handle_save_or_restore_failure(
                evaluate_result,
                i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
                save_file);
            return;
          }

          hst_ptr++;
        }

        hst_ptr = outputhistory[0]->z_history_buffer_start;
      }

      while (hst_ptr != outputhistory[0]->z_history_buffer_front_index)
      {
        if (write_four_byte_number(*hst_ptr, save_file) != 0)
        {
          _handle_save_or_restore_failure(
              evaluate_result,
              i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
              save_file);
          return;
        }

        hst_ptr++;
      }

      if (end_current_chunk(save_file) != 0)
      {
        _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
            save_file);
        return;
      }
    }
#endif // DISABLE_OUTPUT_HISTORY

    if (close_simple_iff_file(save_file) != -0)
    {
      _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_WRITING_SAVE_FILE,
          save_file);
      return;
    }
  }

  if (bool_equal(evaluate_result, true))
    _store_save_or_restore_result(1);
}


void opcode_save_0op(void)
{
  TRACE_LOG("Opcode: SAVE.\n");

  if (strcmp(get_configuration_value("disable-save"), "true") == 0)
    _store_save_or_restore_result(0);
  else
    save_game(
        0,
        (uint16_t)(active_z_story->dynamic_memory_end - z_mem + 1),
        NULL,
        false,
        true,
        get_configuration_value("savegame-path"));
}


void opcode_save_ext(void)
{
  uint16_t address;
  uint16_t length;
  char *filename = NULL;
  long memsize;
  uint8_t *ptr;
  int i;

  TRACE_LOG("Opcode: SAVE_EXT.\n");
  TRACE_LOG("Operands provided: %d.\n", number_of_operands);

  if (strcmp(get_configuration_value("disable-save"), "true") == 0)
  {
    _store_save_or_restore_result(0);
    return;
  }

  if (number_of_operands >= 2)
  {
    address = op[0];
    length = op[1];

    if (number_of_operands >= 3)
    {
      ptr = z_mem + (uint16_t)op[2];
      memsize = (*ptr) + 1;
      filename = (char*)fizmo_malloc(memsize);

      for (i=0; i<*ptr; i++)
        filename[i] = ptr[i+1];
      filename[i] = 0;
    }
  }
  else
  {
    address = 0;
    length = (uint16_t)(active_z_story->dynamic_memory_end - z_mem + 1);
  }

  save_game(
      address,
      length,
      filename,
      false,
      true,
      get_configuration_value("savegame-path"));

  if (filename != NULL)
    free(filename);
}


// Returns 0 for failure,  1 for "save succeeded" and 2 for "the game is
// being restored and is resuming execution again from here, the point where
// it was saved".
int restore_game(uint16_t address, uint16_t length, char *filename, 
    bool skip_asking_for_filename, bool evaluate_result, char *directory)
{
  uint8_t release_number[2];
  uint8_t serial_number[6];
  uint8_t checksum[2];
  uint8_t pc_on_restore_data[3];
  uint32_t pc_on_restore;
  FILE *iff_file;
  uint8_t *dynamic_index;
  int bytes_read;
  int chunk_length;
  uint16_t stack_word;
  uint8_t previous_number_of_locals;
  uint16_t previous_stack_usage;
  int data, data2;
  int copylength;
  uint8_t *restored_story_mem;
  uint8_t *ptr;
  char *system_filename;
  struct z_stack_container *saved_stack;
  uint32_t stack_frame_return_pc;
  bool stack_frame_discard_result;
  uint8_t stack_frame_result_var;
  uint8_t stack_frame_argument_mask;
  uint8_t stack_frame_arguments_supplied;
  // The following four variables are set to 0 to avoid compiler warnings.
  uint8_t current_stack_frame_nof_locals = 0;
  uint8_t last_stack_frame_nof_locals = 0;
  uint16_t current_stack_frame_nof_functions_stack_words = 0;
  uint16_t last_stack_frame_nof_functions_stack_words = 0;
  char *str;
  int i;
#ifndef DISABLE_OUTPUT_HISTORY
  z_ucs history_buffer[HISTORY_BUFFER_INPUT_SIZE];
  int history_input_index;
  int nof_paragraphs_to_save;
#ifdef ENABLE_TRACING
  z_ucs zucs_char_buffer[2];
#endif // ENABLE_TRACING
#endif // DISABLE_OUTPUT_HISTORY

  TRACE_LOG("Restore %d bytes to address %d.\n", length, address);

  if (filename != NULL)
  {
    if (bool_equal(skip_asking_for_filename, true))
    {
      system_filename = filename;
    }
    else
    {
      if ((ask_for_filename(NULL)) < 0)
      {
        if (bool_equal(evaluate_result, true))
          return _store_save_or_restore_result(0);
      }
      system_filename = dup_zucs_string_to_utf8_string(last_savegame_filename);
    }
  }
  else
  {
    if ((ask_for_filename(NULL)) < 0)
    {
      if (bool_equal(evaluate_result, true))
        return _store_save_or_restore_result(0);
    }
    system_filename = dup_zucs_string_to_utf8_string(last_savegame_filename);
  }

  if (directory != NULL)
  {
    str = fizmo_malloc(strlen(directory) + strlen(system_filename) + 2);
    strcpy(str, directory);
    strcat(str, "/");
    strcat(str, system_filename);
  }
  else
    str = system_filename;

  iff_file = open_simple_iff_file(
      str,
      IFF_MODE_READ);

  if (iff_file == NULL)
  {
    if (i18n_translate(
          libfizmo_module_name,
          i18n_libfizmo_COULD_NOT_OPEN_FILE_NAMED_P0S, str)
        == (size_t)-1)
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

    if (directory != NULL)
      free(str);
    free(system_filename);
    if (bool_equal(evaluate_result, true))
      return _store_save_or_restore_result(0);
  }

  if (directory != NULL)
    free(str);

  if ( (filename == NULL) || (bool_equal(skip_asking_for_filename, false)) )
    free(system_filename);

  if (find_chunk("IFhd", iff_file) == -1)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_CANT_FIND_CHUNK_IFHD, iff_file);

  // Skip length code
  if (fseek(iff_file, 4, SEEK_CUR) != 0)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);

  if (fread(release_number, 2, 1, iff_file) != 1)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_COULD_NOT_READ_RELEASE_NUMBER,
        iff_file);

  if (fread(serial_number, 6, 1, iff_file) != 1)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_COULD_NOT_READ_SERIAL_NUMBER,
        iff_file);

  if (fread(checksum, 2, 1, iff_file) != 1)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_COULD_NOT_READ_CHECKSUM, iff_file);

  if (
      (memcmp(release_number, z_mem+2, 2) != 0)
      ||
      (memcmp(serial_number, z_mem+0x12, 6) != 0)
      ||
      (memcmp(checksum, z_mem+0x1c, 2) != 0)
     )
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_RELEASE_NR_SERIAL_NR_OR_CHECKSUM_DOESNT_MATCH,
        iff_file);

  if (fread(pc_on_restore_data, 3, 1, iff_file) != 1)
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_COULD_NOT_READ_RESTORE_PC, iff_file);

  pc_on_restore = 0;
  pc_on_restore |= pc_on_restore_data[0] << 16;
  pc_on_restore |= pc_on_restore_data[1] << 8;
  pc_on_restore |= pc_on_restore_data[2];

  TRACE_LOG("PC on restore: $%x.\n", (unsigned)pc_on_restore);
  pc = z_mem + pc_on_restore;

  TRACE_LOG("Allocating %d bytes for restored dynamic memory.\n", length);

  restored_story_mem = (uint8_t*)fizmo_malloc(length);

  dynamic_index = restored_story_mem;

  if (find_chunk("CMem", iff_file) == 0)
  {
    if (read_chunk_length(iff_file) == -1)
    {
      free(restored_story_mem);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_CANT_READ_CHUNK_LENGTH, iff_file);
    }

    chunk_length = get_last_chunk_length();

    TRACE_LOG("CMem has chunk length %d.\n", chunk_length);

    if (
        (active_z_story->z_file == NULL)
        ||
        (fseek(
                active_z_story->z_file,
                active_z_story->story_file_exec_offset,
                SEEK_SET)
         != 0)
       )
    {
      if (i18n_translate(
            libfizmo_module_name,
            i18n_libfizmo_COULD_NOT_FIND_ORIGINAL_STORY_FILE_P0S,
            active_z_story->absolute_file_name) == (size_t)-1)
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

      free(restored_story_mem);
      return _handle_save_or_restore_failure(evaluate_result, -1, iff_file);
    }

    bytes_read = 0;
    while (bytes_read < chunk_length)
    {
      // Read data from CMem chunk.
      data = fgetc(iff_file);
      bytes_read++;

      if (data == EOF)
      {
        free(restored_story_mem);
        return _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
      }

      if (data != 0)
      {
        data2 = fgetc(active_z_story->z_file);
        // Found content difference to original story file.

        if (data2 == EOF)
        {
          free(restored_story_mem);
          return _handle_save_or_restore_failure(evaluate_result,
              i18n_libfizmo_FATAL_ERROR_READING_STORY_FILE,
              iff_file);
        }

        TRACE_LOG("Altered byte at offset %ld.\n",
            (long int)(dynamic_index - restored_story_mem));

        TRACE_LOG("CMem-Data: %x, Story-Data: %x.\n", data, data2);

        *dynamic_index = (uint8_t)(data2 ^ data);
        dynamic_index++;
      }
      else
      {
        // Found block identical to story file.

        data = fgetc(iff_file);
        bytes_read++;
        if (data == EOF)
        {
          free(restored_story_mem);
          return _handle_save_or_restore_failure(evaluate_result,
              i18n_libfizmo_ERROR_READING_SAVE_FILE,
              iff_file);
        }
        copylength = data + 1;

        //TRACE_LOG("Skipping %d equal bytes.\n", copylength);

        for (i=0; i<copylength; i++)
        {
          data2 = fgetc(active_z_story->z_file);
          if (data2 == EOF)
          {
            free(restored_story_mem);
            return _handle_save_or_restore_failure(evaluate_result,
                i18n_libfizmo_FATAL_ERROR_READING_STORY_FILE,
                iff_file);
          }
          *dynamic_index = (uint8_t)data2;
          dynamic_index++;
        }
      }
    }
    TRACE_LOG("Successfully read %d bytes, uncompressed: %ld.\n",
        bytes_read, (long int)(dynamic_index - restored_story_mem + 1));

    ptr = restored_story_mem + length;

    while (dynamic_index != ptr)
    {
      data = fgetc(active_z_story->z_file);
      if (data == EOF)
      {
        free(restored_story_mem);
        return _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_FATAL_ERROR_READING_STORY_FILE,
            iff_file);
      }
      *dynamic_index = (uint8_t)data;
      dynamic_index++;
    }

    TRACE_LOG("Filled undefined memory with source file up to byte: %ld.\n",
        (long int)(dynamic_index - restored_story_mem));
  }
  else if (find_chunk("UMem", iff_file) == 0)
  {
    if (read_chunk_length(iff_file) == -1)
    {
      free(restored_story_mem);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_CANT_READ_CHUNK_LENGTH, iff_file);
    }

    chunk_length = get_last_chunk_length();
    if (chunk_length < length)
    {
      // Not enough bytes saved.
      free(restored_story_mem);
      return _handle_save_or_restore_failure(evaluate_result, -1, iff_file);
    }

    TRACE_LOG("Chunk length: %d, length-to-read: %d.\n",
        chunk_length, length);

    ptr = restored_story_mem + length;
    while (dynamic_index != ptr)
    {
      data = fgetc(iff_file);
      if (data == EOF)
      {
        free(restored_story_mem);
        return _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_READING_SAVE_FILE,
            iff_file);
      }
      *dynamic_index = (uint8_t)data;
      dynamic_index++;
    }
  }
  else
  {
    //FIXME: Rename error.
    free(restored_story_mem);
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_CANT_FIND_CMEM_OR_UMEM_CHUNK, iff_file);
  }

  if (find_chunk("Stks", iff_file) == -1)
  {
    free(restored_story_mem);
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_CANT_FIND_CHUNK_STKS, iff_file);
  }

  if (read_chunk_length(iff_file) == -1)
  {
    free(restored_story_mem);
    return _handle_save_or_restore_failure(evaluate_result,
        i18n_libfizmo_CANT_READ_CHUNK_LENGTH, iff_file);
  }

  chunk_length = get_last_chunk_length();

  // We create a new stack to store the now incoming data, but keep a
  // reference to the saved stack in "saved_stack", which allows us to
  // re-use it in case somwthing goes wrong during restore.
  saved_stack = create_new_stack();

  bytes_read = 0;
  previous_number_of_locals = 0;
  previous_stack_usage = 0;
  number_of_stack_frames = 0;

  while (bytes_read < chunk_length)
  {
    // Each while iteration processes a single stack frame.

    // PC Bits 16-23
    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_return_pc = (data & 0xff) << 16;

    // PC Bits 8-15
    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_return_pc |= (data & 0xff) << 8;

    // PC Bits 0-7
    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_return_pc |= (data & 0xff);

    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_discard_result = ((data & 0x10) != 0 ? true : false);
    current_stack_frame_nof_locals = (data & 0xf);

    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_result_var = (data & 0xff);

    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    stack_frame_argument_mask = (data & 0xff);

    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    current_stack_frame_nof_functions_stack_words = ((data & 0xff) << 8);

    data = fgetc(iff_file);
    if (data == EOF)
    {
      free(restored_story_mem);
      restore_old_stack(saved_stack);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
    }
    current_stack_frame_nof_functions_stack_words |= (data & 0xff);

    bytes_read += 8;

    stack_frame_arguments_supplied = 0;
    while (stack_frame_argument_mask != 0)
    {
      stack_frame_arguments_supplied++;
      stack_frame_argument_mask >>= 1;
    }

    if (number_of_stack_frames == 0)
      store_first_stack_frame();
    else
      store_followup_stack_frame_header(
          last_stack_frame_nof_locals,
          stack_frame_discard_result,
          stack_frame_arguments_supplied,
          last_stack_frame_nof_functions_stack_words,
          stack_frame_return_pc,
          stack_frame_result_var);

    i = 0;
    // write locals and stack
    while (i < current_stack_frame_nof_locals
        + current_stack_frame_nof_functions_stack_words)
    {
      data = fgetc(iff_file);
      if (data == EOF)
      {
        free(restored_story_mem);
        restore_old_stack(saved_stack);
        return _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
      }
      stack_word = (data & 0xff) << 8;

      data = fgetc(iff_file);
      if (data == EOF)
      {
        free(restored_story_mem);
        restore_old_stack(saved_stack);
        return _handle_save_or_restore_failure(evaluate_result,
            i18n_libfizmo_ERROR_READING_SAVE_FILE, iff_file);
      }
      stack_word |= (data & 0xff);

      z_stack_push_word(stack_word);

      i++;
    }
    bytes_read += i * 2;

    last_stack_frame_nof_locals
      = current_stack_frame_nof_locals;

    last_stack_frame_nof_functions_stack_words
      = current_stack_frame_nof_functions_stack_words;

    number_of_stack_frames++;
  }
  TRACE_LOG("Number of stack frames: %d.\n", number_of_stack_frames);
  nof_paragraphs_to_save = get_paragraph_save_amount();

#ifndef DISABLE_OUTPUT_HISTORY
  if (
      (nof_paragraphs_to_save > 0)
      &&
      (outputhistory[0] != NULL)
      &&
      (find_chunk("TxHs", iff_file) == 0)
     )
  {
    if (read_chunk_length(iff_file) == -1)
    {
      free(restored_story_mem);
      return _handle_save_or_restore_failure(evaluate_result,
          i18n_libfizmo_CANT_READ_CHUNK_LENGTH, iff_file);
    }

    chunk_length = get_last_chunk_length();
    TRACE_LOG("saved history size: %d bytes.\n", chunk_length);

#ifdef ENABLE_TRACING
    zucs_char_buffer[1] = 0;
    i = 0;
#endif // ENABLE_TRACING

    if (chunk_length > 0)
    {
      history_input_index = 0;
      while (chunk_length != 0)
      {
        history_buffer[history_input_index]
          = (z_ucs)read_four_byte_number(iff_file);

#ifdef ENABLE_TRACING
        zucs_char_buffer[0] = history_buffer[history_input_index];
        TRACE_LOG("read char %d: \"", i);
        TRACE_LOG_Z_UCS(zucs_char_buffer);
        TRACE_LOG("\"\n");
        i++;
#endif // ENABLE_TRACING

        chunk_length -= 4;
        history_input_index++;

        if (
            (history_input_index == HISTORY_BUFFER_INPUT_SIZE - 1)
            ||
            (chunk_length == 0)
           )
        {
          store_data_in_history(
              outputhistory[0], history_buffer, history_input_index, true);
          history_input_index = 0;
        }
      }

      //active_interface->game_was_restored_and_history_modified();
    }
  }
#endif // DISABLE_OUTPUT_HISTORY

  if (fclose(iff_file) != 0)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_FUNCTION_CALL_P0S_ABORTED_DUE_TO_ERROR,
        -0x0100,
        "fclose");

  number_of_locals_active
    = current_stack_frame_nof_locals;
  stack_words_from_active_routine
    = current_stack_frame_nof_functions_stack_words;

  local_variable_storage_index
    = z_stack_index
    - current_stack_frame_nof_locals
    - current_stack_frame_nof_functions_stack_words;

  TRACE_LOG(
      "Restored stack: %d locals active, %d, words from active routine.\n",
      number_of_locals_active, stack_words_from_active_routine);

  if (saved_stack->z_stack != NULL)
    free(saved_stack->z_stack);
  free(saved_stack);

  // restored_story_mem has been filled via dynamix_index above so inhibit
  // warning is okay.

  /*@-compdef@*/
  memcpy(
      z_mem + address,
      restored_story_mem,
      length);
  /*@+compdef@*/

  free(restored_story_mem);

#ifdef ENABLE_TRACING
  dump_stack_to_tracelog();
  dump_dynamic_memory_to_tracelog();
#endif // ENABLE_TRACING

  if (bool_equal(evaluate_result, true))
    _store_save_or_restore_result(2);

  return 2;
}


void opcode_restore_0op(void)
{
  TRACE_LOG("Opcode: RESTORE.\n");

  if (strcmp(get_configuration_value("disable-restore"), "true") == 0)
    _store_save_or_restore_result(0);
  else
    restore_game(
        0,
        (uint16_t)(active_z_story->dynamic_memory_end - z_mem + 1),
        NULL,
        false,
        true,
        get_configuration_value("savegame-path"));
}


void opcode_restore_ext(void)
{
  uint16_t address;
  uint16_t length;
  uint8_t *ptr;
  char *filename = NULL;
  long memsize;
  int i;

  TRACE_LOG("Opcode: RESTORE_EXT.\n");

  if (strcmp(get_configuration_value("disable-restore"), "true") == 0)
  {
    _store_save_or_restore_result(0);
    return;
  }

  if (number_of_operands >= 2)
  {
    address = op[0];
    length = op[1];

    if (number_of_operands >= 3)
    {
      ptr = z_mem + (uint16_t)op[2];
      memsize = *ptr + 1;
      filename = (char*)fizmo_malloc(memsize);

      for (i=0; i<*ptr; i++)
        filename[i] = ptr[i+1];
      filename[i] = 0;
    }
  }
  else
  {
    address = 0;
    length = (uint16_t)(active_z_story->dynamic_memory_end - z_mem + 1);
  }

  restore_game(
      address,
      length,
      filename,
      false,
      true,
      get_configuration_value("savegame-path"));

  if (filename != NULL)
    free(filename);
}


bool detect_saved_game(char *file_to_check, char **story_file_to_load)
{
  FILE *iff_file = open_simple_iff_file(file_to_check, IFF_MODE_READ);
  uint8_t release_number_buf[2];
  uint16_t release_number;
  char serial_number[7];
  uint8_t checksum_buf[2];
  uint16_t checksum;
  struct z_story_list_entry *story_entry;

  if (iff_file == NULL)
  {
    return false;
  }
  else if (find_chunk("RIdx", iff_file) == 0)
  {
    // We have an Blorb-IFF-file, no savegame.
    fclose(iff_file);
    return false;
  }
  else
  {
    if (find_chunk("IFhd", iff_file) == -1)
    {
      fclose(iff_file);
      return false;
    }

    // Skip length code
    if (fseek(iff_file, 4, SEEK_CUR) != 0)
    {
      fclose(iff_file);
      return false;
    }

    if (fread(release_number_buf, 2, 1, iff_file) != 1)
    {
      fclose(iff_file);
      return false;
    }

    if (fread(serial_number, 6, 1, iff_file) != 1)
    {
      fclose(iff_file);
      return false;
    }

    if (fread(checksum_buf, 2, 1, iff_file) != 1)
    {
      fclose(iff_file);
      return false;
    }

    fclose(iff_file);

    release_number = (release_number_buf[0] << 8) | release_number_buf[1];
    serial_number[6] = 0;
    checksum = (checksum_buf[0] << 8) | checksum_buf[1];

    if ((story_entry = get_z_story_entry_from_list(
            serial_number, release_number, checksum)) != NULL)
    {
      *story_file_to_load = fizmo_strdup(story_entry->filename);
      free_z_story_list_entry(story_entry);
    }

    return true;
  }
}



#endif /* savegame_c_INCLUDED */

