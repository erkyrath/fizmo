
/* wordwrap.c
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


#ifndef wordwrap_c_INCLUDED
#define wordwrap_c_INCLUDED

// The wordwrapper wraps words by using a space character as seperator only.
// The details of word-wrapping are not specified in the Z-Machine
// specification, so I'll not designate other characters than the space
// as seperators.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../tools/tracelog.h"
#include "../tools/types.h"
#include "../tools/i18n.h"
#include "../tools/z_ucs.h"
#include "wordwrap.h"
#include "fizmo.h"
#include "hyphenation.h"


static z_ucs word_split_chars[] = {
  Z_UCS_SPACE, Z_UCS_MINUS, Z_UCS_NEWLINE, Z_UCS_DOT, (z_ucs)',',
  (z_ucs)'"', 0 };


/*
struct wordwrap_target
{
  void (*wrapped_text_output_destination)(z_ucs *output, void *parameter);
};


struct wordwrap_metadata
{
  z_ucs *output_index;
  void (*metadata_output_function)(void *ptr_parameter, uint32_t int_parameter);
  void *ptr_parameter;
  uint32_t int_parameter;
};
*/


WORDWRAP *wordwrap_new_wrapper(size_t line_length,
    void (*wrapped_text_output_destination)(z_ucs *output, void *parameter),
    void *destination_parameter, bool add_newline_after_full_line,
    int left_side_padding, bool flush_after_newline, bool enable_hyphenation)
{
  WORDWRAP *result = fizmo_malloc(sizeof(WORDWRAP));
  int i;

  result->line_length = line_length;
  result->wrapped_text_output_destination = wrapped_text_output_destination;
  result->destination_parameter = destination_parameter;
  result->add_newline_after_full_line = add_newline_after_full_line;
  result->left_side_padding = left_side_padding;
  if (left_side_padding > 0)
  {
    result->padding_buffer
      = fizmo_malloc((left_side_padding + 1) * sizeof(z_ucs));
    for (i=0; i<left_side_padding; i++)
      result->padding_buffer[i] = Z_UCS_SPACE;
    result->padding_buffer[i] = 0;
  }
  else
    result->padding_buffer = NULL;
  result->flush_after_newline = flush_after_newline;
  result->enable_hyphenation = enable_hyphenation;
  result->input_buffer_size = line_length * 4;
  result->input_buffer = fizmo_malloc(sizeof(z_ucs)*result->input_buffer_size);
  result->input_index = 0;
  result->metadata = NULL;
  result->metadata_size = 0;
  result->metadata_index = 0;

  return result;
}


void wordwrap_destroy_wrapper(WORDWRAP *wrapper_to_destroy)
{
  free(wrapper_to_destroy->input_buffer);
  if (wrapper_to_destroy->padding_buffer != NULL)
    free(wrapper_to_destroy->padding_buffer);
  if (wrapper_to_destroy->metadata != NULL)
    free(wrapper_to_destroy->metadata);
  free(wrapper_to_destroy);
}


static void output_buffer(WORDWRAP *wrapper, z_ucs *buffer_start,
    int *metadata_offset)
{
  long index, end_index, next_index;
  struct wordwrap_metadata *metadata_entry;
  z_ucs buf;

  TRACE_LOG("Output buffer: \"");
  TRACE_LOG_Z_UCS(buffer_start);
  TRACE_LOG("\"");

  if (*metadata_offset == wrapper->metadata_index)
  {
    wrapper->wrapped_text_output_destination(
        buffer_start,
        wrapper->destination_parameter);
  }
  else
  {
    index = buffer_start - wrapper->input_buffer;
    end_index = index + z_ucs_len(buffer_start);

    /*
       wrapper->metadata[wrapper->metadata_index].output_index
       = wrapper->input_index;
    */

    while (index < end_index)
    {
      TRACE_LOG("index: %ld, end_index: %ld.\n", index, end_index);
      TRACE_LOG("metadata-offset: %d.\n", *metadata_offset);

      if (*metadata_offset < wrapper->metadata_index)
      {
        TRACE_LOG("next metadata-index: %ld.\n",
            wrapper->metadata[*metadata_offset].output_index);

        while (
            (*metadata_offset < wrapper->metadata_index)
            &&
            (wrapper->metadata[*metadata_offset].output_index == index)
            )
        {
          metadata_entry = &wrapper->metadata[*metadata_offset];

          TRACE_LOG("Output metadata prm %d at %ld.\n",
              metadata_entry->int_parameter,
              index);

          metadata_entry->metadata_output_function(
              metadata_entry->ptr_parameter,
              metadata_entry->int_parameter);

          (*metadata_offset)++;
        }
      }

      next_index
        = (
            (*metadata_offset < wrapper->metadata_index)
            &&
            (wrapper->metadata[*metadata_offset].output_index < end_index)
          )
        ? wrapper->metadata[*metadata_offset].output_index
        : end_index;

      TRACE_LOG("next-index: %ld.\n", next_index);


      buf = wrapper->input_buffer[next_index];
      wrapper->input_buffer[next_index] = 0;

      wrapper->wrapped_text_output_destination(
          wrapper->input_buffer + index,
          wrapper->destination_parameter);

      wrapper->input_buffer[next_index] = buf;

      index = next_index;
    }
  }
}


static void flush_input_buffer(WORDWRAP *wrapper, bool force_flush)
{
  z_ucs *index = NULL, *ptr, *hyphenated_word, *last_hyphen, *word_start;
  z_ucs *word_end_without_split_chars, *word_end_with_split_chars;
  z_ucs buf=0, buf2; // buf initialized to avoid compiler warning
  z_ucs buf3 = '-'; // buf3 initialized to avoid compiler warning
  long len, chars_sent = 0;
  z_ucs *input = wrapper->input_buffer;
  bool minus_found;
  int metadata_offset = 0;
  int i, hyphen_offset, chars_left_on_line;
  struct wordwrap_metadata *metadata_entry;

  input[wrapper->input_index] = 0;
  TRACE_LOG("input-index: %ld\n", wrapper->input_index);

  TRACE_LOG("metadata stored: %d.\n", wrapper->metadata_index);

  for (;;)
  {
    TRACE_LOG("Processing flush.\n");

    if (*input != 0)
    {
      TRACE_LOG("flush wordwrap-buffer: \"");
      TRACE_LOG_Z_UCS(input);
      TRACE_LOG("\".\n");
    }

    if ((index = z_ucs_chr(input, Z_UCS_NEWLINE)) != NULL)
      len = index - input;
    else
    {
      len = z_ucs_len(input);
      TRACE_LOG("len:%ld, force:%d.\n", len, force_flush);

      if (len == 0)
      {
        if (force_flush == true)
        {
          // Force flush metadata behind end of output.
          while (metadata_offset < wrapper->metadata_index)
          {
            TRACE_LOG("flush post-output metadata at: %ld.\n",
                wrapper->metadata[metadata_offset].output_index);

            metadata_entry = &wrapper->metadata[metadata_offset];

            TRACE_LOG("Output metadata prm %d.\n",
                metadata_entry->int_parameter);

            metadata_entry->metadata_output_function(
                metadata_entry->ptr_parameter,
                metadata_entry->int_parameter);

            metadata_offset++;
          }
        }
        break;
      }

      if ( (len <= wrapper->line_length) && (force_flush == false) )
        // We're quitting on len == wrapper->line_length since we can only
        // determine wether we can break cleanly is if a space follows
        // immediately after the last char.
        break;

      // FIXME: Add break in case hyph is enabled and a word longer than
      // the line is not terminated with a space.
    }

    TRACE_LOG("wordwrap-flush-len: %ld.\n", len);

    if (len <= wrapper->line_length)
    {
      // Line fits on screen.
      TRACE_LOG("Line fits on screen.\n");
      if (index != NULL)
      {
        index++;
        len++;
        buf = *index;
        *index = 0;
      }
      chars_sent += len;

      output_buffer(wrapper, input, &metadata_offset);
      if (wrapper->left_side_padding != 0)
        wrapper->wrapped_text_output_destination(
            wrapper->padding_buffer,
            wrapper->destination_parameter);

      if (index != NULL)
        *index = buf;
      else
      {
        //wrapper->input_index = 0;
        break;
      }
    }
    else if (wrapper->enable_hyphenation == true)
    {
      TRACE_LOG("to wrap/hyphenate (force:%d): \"", force_flush);
      TRACE_LOG_Z_UCS(input);
      TRACE_LOG("\".\n");

      if ((word_end_without_split_chars = z_ucs_chrs(
              input + wrapper->line_length, word_split_chars)) == NULL)
      {
        if (force_flush == true)
          word_end_without_split_chars = input + len;
        else
        {
          TRACE_LOG("No word end found.\n");
          break;
        }
      }

      // We'll remeber the current position of the word-split-char we've
      // just found in "word_end_without_split_chars" and advance this pointer
      // until we won't find any more word-split-chars. Thus, if we've just
      // found the start dot of an ellipsis "..." we'll end up with the
      // position of the last dot.
      word_end_with_split_chars = word_end_without_split_chars;
      while ((ptr = z_ucs_chr(
              word_split_chars, *word_end_with_split_chars)) != NULL)
        word_end_with_split_chars++;
      // Now we've stored the end of the word in "word_end_without_split_chars".

      // We've now found a word boundary. This char may however not be part
      // of the actual last word (we might have hit a minus representing
      // a n- or m-dash--like this--or an elipsis like "..."). So we'll
      // rewind further if necessary.
      TRACE_LOG("examining split end:\"%c\".\n", *word_end_without_split_chars);
      //while (word_end_without_split_chars - 1 > input + wrapper->line_length)
      while (word_end_without_split_chars > input)
      {
        TRACE_LOG("Checking for split-char: %c/%d.\n",
            *word_end_without_split_chars, *word_end_without_split_chars);
        if (z_ucs_chr(
              word_split_chars, *(word_end_without_split_chars - 1)) == NULL)
          break;
        word_end_without_split_chars--;
      }

      if (
          (word_end_without_split_chars != word_end_with_split_chars)
          &&
          (*word_end_without_split_chars == Z_UCS_SPACE)
          &&
          (word_end_without_split_chars < input + wrapper->line_length)
         )
      {
        // In this case we have skipped a number of stand-alone word-split-
        // chars (like the elipsis in "Powers -- these") and are now so far
        // back that we can split right at the space before these chars.
        index = word_end_without_split_chars;
        last_hyphen = NULL;
      }
      else
      {
        word_start = NULL;

        /*
        TRACE_LOG("examining split end:\"%c\".\n",
            *word_end_without_split_chars);
        while (word_end_without_split_chars > input)
        {
          TRACE_LOG("Checking for split-char: %c/%d.\n",
              *word_end_without_split_chars, *word_end_without_split_chars);
          if (z_ucs_chr(
                word_split_chars, *(word_end_without_split_chars - 1)) == NULL)
            break;
          word_end_without_split_chars--;
        }
        */

        // In case we've now found a word end, check for dashes before it.
        // Example: "first-class car", where the word end we've now found is
        // between "first-class" and "car".
        word_start = word_end_without_split_chars - 1;
        while (word_start > input)
        {
          TRACE_LOG("examining word end: \"%c\".\n", *word_start);
          if (*word_start == Z_UCS_MINUS)
          {
            if (input + wrapper->line_length > word_start)
            {
              // Found a dash to break on
              word_start++;
              break;
            }
          }
          else if ((ptr = z_ucs_chr(word_split_chars, *word_start)) != NULL)
          {
            // In case we've found a non-dash separator, we've found the
            // start of the word.
            word_start++;
            break;
          }
          word_start--;
        }

        // FIXME: Do we need a space left from here?

        TRACE_LOG("word-start: %c\n", *word_start);

        TRACE_LOG("%p / %p\n", word_end_without_split_chars,
            word_end_with_split_chars);

        TRACE_LOG("%c / %c\n", *word_end_without_split_chars,
            *word_end_with_split_chars);

        TRACE_LOG("%p / %p\n",
            word_end_without_split_chars, input + wrapper->line_length);

        /*
           if (
           (word_end_without_split_chars == word_end_with_split_chars + 1)
           &&
           (*word_end_without_split_chars == Z_UCS_MINUS)
           &&
           (word_end_without_split_chars < input + wrapper->line_length)
           )
           {
        // We've hit a minus in the middle of a word, like "far-off".
        TRACE_LOG("minus in word.\n");
        index = word_end_without_split_chars+1;
        last_hyphen = NULL;
        }
        */

        last_hyphen = NULL;

        if (word_end_with_split_chars > input + wrapper->line_length)
        {
          // We only have to hyphenate in case the line is still too long.

          buf = *word_end_without_split_chars;
          *word_end_without_split_chars = 0;

          TRACE_LOG("buffer terminated at word end: \"");
          TRACE_LOG_Z_UCS(input);
          TRACE_LOG("\".\n");

          index = word_start;

          if ((hyphenated_word = hyphenate(index)) == NULL)
          {
            TRACE_LOG("Error hyphenating.\n");
            exit(-1);
          }
          *word_end_without_split_chars = buf;

          chars_left_on_line = wrapper->line_length - (index - input);
          TRACE_LOG("chars left on line: %d\n", chars_left_on_line);

          hyphen_offset = index - input;

          ptr = hyphenated_word;
          while (chars_left_on_line > 0)
          {
            TRACE_LOG("Testing %c for hyphen.\n", *ptr);
            if (*ptr == Z_UCS_SOFT_HYPEN)
              last_hyphen = input + (wrapper->line_length - chars_left_on_line);
            else
              chars_left_on_line--;
            ptr++;
          }
          free(hyphenated_word);

          if (last_hyphen != NULL)
          {
            TRACE_LOG("Last hyphen at %ld.\n", last_hyphen - input);
            buf3 = *last_hyphen;
            *last_hyphen = '-';

            index = last_hyphen + 1;
          }
          else if ( (word_start != input) && (*(word_start-1) != Z_UCS_MINUS) )
          {
            TRACE_LOG("No hyphen found.\n");
            index--;
          }
        }
        else
        {
          index = word_end_with_split_chars;
        }
      }

      // Output everything before *index and a newline after.

      buf2 = *index;
      *index = Z_UCS_NEWLINE;
      buf = *(index + 1);
      *(index + 1) = 0;

      output_buffer(wrapper, input, &metadata_offset);
      if (wrapper->left_side_padding != 0)
        wrapper->wrapped_text_output_destination(
            wrapper->padding_buffer,
            wrapper->destination_parameter);

      *(index + 1) = buf;
      *index = buf2;

      if (last_hyphen != NULL)
      {
        *last_hyphen = buf3;
        index--;
      }

      if (*index == Z_UCS_SPACE)
        index++;

      len = index - input;
      chars_sent += len;

      TRACE_LOG("Processed %ld chars in hyphenated wordwrap.\n", len);
    }
    else
    {
      // Line won't fit completely. Find the end of the last word.
      TRACE_LOG("linelength: %d.\n", wrapper->line_length);

      //word_end_with_split_chars = NULL;
      ptr = input + wrapper->line_length;
      if (*ptr == Z_UCS_SPACE)
        ptr++;
      TRACE_LOG("%p/%p/%c\n", input, ptr, *ptr);
      buf = *ptr;
      *ptr = 0;
      TRACE_LOG("examine buffer: \"");
      TRACE_LOG_Z_UCS(input);
      TRACE_LOG("\".\n");

      index = z_ucs_rchrs(input, word_split_chars);
      if (*index == Z_UCS_MINUS)
      {
        minus_found = true;
        index++;
      }
      else
        minus_found = false;
      TRACE_LOG("Found word-split at %p from %p.\n", index, input);
      *ptr = buf;

      if (index == NULL)
        index = input + wrapper->line_length;

      buf = *index;
      *index = Z_UCS_NEWLINE;
      buf2 = *(index+1);
      *(index+1) = 0;

      TRACE_LOG("Output from %p.\n", input);

      output_buffer(wrapper, input, &metadata_offset);
      if (wrapper->left_side_padding != 0)
        wrapper->wrapped_text_output_destination(
            wrapper->padding_buffer,
            wrapper->destination_parameter);

      *(index+1) = buf2;
      *index = buf;

      if (minus_found == false)
        index++;

      len = index - input;
      chars_sent += len;
    }

    TRACE_LOG("len-after: %ld.\n", len);

    if (index != NULL)
    {
      TRACE_LOG("index: \"");
      TRACE_LOG_Z_UCS(index);
      TRACE_LOG("\".\n");
    }

    input += len;
  }

  TRACE_LOG("chars sent: %ld, moving: %ld.\n",
      chars_sent, wrapper->input_index - chars_sent + 1);

  memmove(
      wrapper->input_buffer,
      input,
      sizeof(z_ucs) * (wrapper->input_index - chars_sent + 1));

  wrapper->input_index -= chars_sent;

  if (metadata_offset > 0)
  {
    memmove(
        wrapper->metadata,
        wrapper->metadata + metadata_offset,
        sizeof(struct wordwrap_metadata)
        * (wrapper->metadata_index - metadata_offset));
    wrapper->metadata_index -= metadata_offset;

    TRACE_LOG("metadata stored: %d.\n", wrapper->metadata_index);
  }

  for (i=0; i<wrapper->metadata_index; i++)
    wrapper->metadata[i].output_index -= chars_sent;
}


void wordwrap_wrap_z_ucs(WORDWRAP *wrapper, z_ucs *input)
{
  size_t len, chars_to_copy, space_in_buffer;

  len = z_ucs_len(input);

  while (len > 0)
  {
    space_in_buffer = wrapper->input_buffer_size - 1 - wrapper->input_index;

    chars_to_copy
      = len > space_in_buffer
      ? space_in_buffer
      : len;

    z_ucs_ncpy(
        wrapper->input_buffer + wrapper->input_index,
        input,
        chars_to_copy);

    wrapper->input_index += chars_to_copy;
    wrapper->input_buffer[wrapper->input_index] = 0;
    input += chars_to_copy;
    len -= chars_to_copy;

    //TRACE_LOG("chars copied: %d, chars left: %ld.\n",chars_to_copy,(long)len);

    if (
        (wrapper->input_index == wrapper->input_buffer_size - 1)
        ||
        (
         (wrapper->flush_after_newline == true)
         &&
         (
          (wrapper->input_index > wrapper->line_length)
          ||
          (z_ucs_chr(wrapper->input_buffer, Z_UCS_NEWLINE) != NULL)
         )
        )
       )
    {
      flush_input_buffer(wrapper, false);

      //FIXME: Increase buffer size in case flush not possible.
    }
  }
}


void wordwrap_flush_output(WORDWRAP *wrapper)
{
  flush_input_buffer(wrapper, true);
}


void wordwrap_insert_metadata(WORDWRAP *wrapper,
    void (*metadata_output)(void *ptr_parameter, uint32_t int_parameter),
    void *ptr_parameter, uint32_t int_parameter)
{
  size_t bytes_to_allocate;

  // Before adding new metadata, check if we need to allocate more space.
  if (wrapper->metadata_index == wrapper->metadata_size)
  {
    bytes_to_allocate
      = (size_t)(
          (wrapper->metadata_size + 32) * sizeof(struct wordwrap_metadata));

    TRACE_LOG("Allocating %d bytes for wordwrap-metadata.\n",
        (int)bytes_to_allocate);

    wrapper->metadata = (struct wordwrap_metadata*)fizmo_realloc(
        wrapper->metadata, bytes_to_allocate);

    wrapper->metadata_size += 32;

    TRACE_LOG("Wordwrap-metadata at %p.\n", wrapper->metadata);
  }

  TRACE_LOG("Current wordwrap-metadata-index is %d.\n",
      wrapper->metadata_index);

  TRACE_LOG("Current wordwrap-metadata-entry at %p.\n",
      &(wrapper->metadata[wrapper->metadata_index]));

  wrapper->metadata[wrapper->metadata_index].output_index
    = wrapper->input_index;
  wrapper->metadata[wrapper->metadata_index].metadata_output_function
    = metadata_output;
  wrapper->metadata[wrapper->metadata_index].ptr_parameter
    = ptr_parameter;
  wrapper->metadata[wrapper->metadata_index].int_parameter
    = int_parameter;

  TRACE_LOG("Added new metadata entry at %ld with int-parameter %ld, ptr:%p.\n",
      wrapper->input_index, (long int)int_parameter, ptr_parameter);

  wrapper->metadata_index++;
}


/*
void wordwrap_set_line_index(WORDWRAP *wrapper, int new_line_index)
{
}
*/


void wordwrap_output_left_padding(WORDWRAP *wrapper)
{
  if (wrapper->left_side_padding > 0)
    wrapper->wrapped_text_output_destination(
        wrapper->padding_buffer,
        wrapper->destination_parameter);
}


void wordwrap_adjust_line_length(WORDWRAP *wrapper, size_t new_line_length)
{
  wrapper->line_length = new_line_length;
}


#endif /* wordwrap_c_INCLUDED */

