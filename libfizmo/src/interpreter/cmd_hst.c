
/* cmd_hst.c
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


#ifndef cmd_hst_c_INCLUDED
#define cmd_hst_c_INCLUDED

#define _XOPEN_SOURCE_EXTENDED 1
#define _XOPEN_SOURCE 1
// __STDC_ISO_10646__

#include <stdlib.h>
#include <string.h>

#include "cmd_hst.h"
#include "fizmo.h"
#include "../tools/tracelog.h"
#include "../tools/types.h"
#include "../tools/i18n.h"


/*@-exportlocal@*/
/*@null@*/ /*@only@*/ zscii *command_history_buffer = NULL;
int command_history_nof_entries = 0;
size_t command_history_entries[NUMBER_OF_REMEMBERED_COMMANDS];
int command_history_newest_entry = 0;
int command_history_oldest_entry = 0;
/*@-exportlocal@*/

static size_t command_history_buffer_size = 0;
static int command_history_highest_entry = 0;
static int command_history_last_entry = 0;


static void command_history_increase_buffer(size_t input_size)
{
  size_t size_increase = 0;
  int i;

  while (size_increase < input_size)
  {
    size_increase += COMMAND_HISTORY_SIZE_INCREASE;
  }

  command_history_buffer_size += size_increase;

  command_history_buffer
    = (zscii*)fizmo_realloc(
        command_history_buffer, command_history_buffer_size * sizeof(zscii));

  TRACE_LOG("Resized command history to %zu bytes at %p-%p.\n",
      command_history_buffer_size,
      command_history_buffer,
      command_history_buffer + command_history_buffer_size);

  TRACE_LOG("Oldest entry: %d(%zu).\n", command_history_oldest_entry,
      command_history_entries[command_history_oldest_entry]);
  TRACE_LOG("Newest entry: %d(%zu).\n", command_history_newest_entry,
      command_history_entries[command_history_newest_entry]);
  TRACE_LOG("Highest entry: %d(%zu).\n", command_history_highest_entry,
      command_history_entries[command_history_highest_entry]);

  TRACE_LOG("Moving %ld bytes from %p to %p.\n",
      (long int)((command_history_entries[command_history_highest_entry]
       + strlen((char*)command_history_buffer
         + command_history_entries[command_history_highest_entry]) + 1
       - command_history_entries[command_history_oldest_entry])
      * sizeof(zscii)),

      command_history_buffer
      + command_history_entries[command_history_oldest_entry],

      command_history_buffer
      + command_history_entries[command_history_oldest_entry]
      + size_increase);

  memmove(
      command_history_buffer
      + command_history_entries[command_history_oldest_entry]
      + size_increase,

      command_history_buffer
      + command_history_entries[command_history_oldest_entry],

      (command_history_entries[command_history_highest_entry]
       + strlen((char*)command_history_buffer
         + command_history_entries[command_history_highest_entry]) + 1
       - command_history_entries[command_history_oldest_entry])
      * sizeof(zscii));

  // Now we have to move all entries except the one we're writing to.
  for (i=0; i<NUMBER_OF_REMEMBERED_COMMANDS; i++)
  {
    if (i != command_history_newest_entry)
    {
      command_history_entries[i] += size_increase;

      /*
      TRACE_LOG("Entry %d after move: '%s'.\n",
          i, command_history_buffer + command_history_entries[i]);
      */
    }
  }
}


void store_command_in_history(zscii *new_command)
{
  size_t input_size = strlen((char*)new_command) + 1;
  size_t end_of_used_space;
  size_t current_index;
  int i;

  TRACE_LOG("Command-Buffer at %p-%p.\n",
      command_history_buffer,
      command_history_buffer + command_history_buffer_size);
  TRACE_LOG("Oldest entry: %d.\n", command_history_oldest_entry);
  TRACE_LOG("Newest entry: %d.\n", command_history_newest_entry);
  TRACE_LOG("Highest entry: %d.\n", command_history_highest_entry);

  // First we calculate the space used so far and store it in "i".
  if (command_history_nof_entries > 0)
  {
    end_of_used_space
      = command_history_entries[command_history_highest_entry]
      + strlen((char*)command_history_buffer
          + command_history_entries[command_history_highest_entry])
      + 1;
  }
  else
  {
    end_of_used_space = 0;
  }

  TRACE_LOG("End of used space: %zu.\n", end_of_used_space);
  TRACE_LOG("Input size: %zu.\n", input_size);
  TRACE_LOG("Buffer size: %zu.\n", command_history_buffer_size);

  TRACE_LOG("Commands stored: %d, maximum: %d.\n",
      command_history_nof_entries,
      NUMBER_OF_REMEMBERED_COMMANDS);

  if (command_history_nof_entries < NUMBER_OF_REMEMBERED_COMMANDS)
  {
    // In case we've not yet stored NUMBER_OF_REMEMBERED_COMMANDS commands,
    // we simply allocate space to fit the new line into the buffer. This
    // section is executed in case we haven't allocated any space, so after
    // it's execution we know that command_history_buffer != NULL.
       
    while (end_of_used_space + input_size > command_history_buffer_size)
    {
      command_history_buffer_size += COMMAND_HISTORY_SIZE_INCREASE;

      command_history_buffer
        = (zscii*)realloc(
            command_history_buffer,
            command_history_buffer_size * sizeof(zscii));

      TRACE_LOG("Resized command history to %d bytes at %p-%p.\n",
          (int)command_history_buffer_size,
          command_history_buffer,
          command_history_buffer + command_history_buffer_size);
    }

    command_history_newest_entry = command_history_nof_entries;
    command_history_entries[command_history_nof_entries++] = end_of_used_space;
    command_history_highest_entry = command_history_newest_entry;

    TRACE_LOG("Writing command to %p.\n",
        command_history_buffer + end_of_used_space);

    strcpy(
        (char*)command_history_buffer + end_of_used_space,
        (char*)new_command);
  }
  else
  {
    // If we get here we've already stored the maximum number of entries
    // in the history. We'll now re-use entries in the command_history_entries
    // array.

    command_history_last_entry = command_history_newest_entry;

    // First: adapt indexes, overwrite the oldest entry.
    if (++command_history_oldest_entry == NUMBER_OF_REMEMBERED_COMMANDS)
    {
      command_history_oldest_entry = 0;
    }
    if (++command_history_newest_entry == NUMBER_OF_REMEMBERED_COMMANDS)
    {
      command_history_newest_entry = 0;
    }

    if (command_history_newest_entry == command_history_highest_entry)
    {
      // Here we have to correct the index: Since we're about to
      // re-use the currently highest index, the next-lower one is
      // the new highest.
      if (--command_history_highest_entry < 0)
      {
        command_history_highest_entry = NUMBER_OF_REMEMBERED_COMMANDS - 1;
      }
      TRACE_LOG("Fixed new highest entry to: %d / %p.\n",
          command_history_highest_entry,
          command_history_buffer
          + command_history_entries[command_history_highest_entry]);

      end_of_used_space
        = command_history_entries[command_history_highest_entry]
        + strlen((char*)command_history_buffer
            + command_history_entries[command_history_highest_entry])
        + 1;
    }

    // Now, we've got to check if we're appending at the end of the buffer
    // space and have no more entries "above" the new one, or if we're writing
    // into a gap.
    if (command_history_highest_entry == command_history_last_entry)
    {
      // In this case we're appending at the end of the buffer.
      TRACE_LOG("Trying to append at the end of the buffer.\n");

      if (end_of_used_space + input_size > command_history_buffer_size)
      {
        // In this case we don't have enough memory to store the next history
        // line and keep all other entries. But since we've already stored the
        // requested maximum number of entries, we'll now wrap around and
        // start re-using the buffer from the bottom. In case there's enough
        // space there we don't have to allocate or move around anything.
        TRACE_LOG("Not enough space in buffer to append at the end.\n");

        // The new entry will start at the bottom of the buffer.
        command_history_entries[command_history_newest_entry] = 0;

        if (input_size > command_history_entries[command_history_oldest_entry])
        {
          // Not lucky: We don't have enough space to keep all entries
          // alive. We'll have to allocate more and move all entries
          // above this one upwards.
          TRACE_LOG("Not enough space at buffer-start.\n");
          command_history_increase_buffer(input_size);

          TRACE_LOG("Writing command to %p.\n", command_history_buffer);

          // Now we have enough space to store the new command. Due to the
          // "if (command_history_nof_entries < NUMBER_OF_REMEMBERED_COMMANDS)"
          // check above we're sure that command_history_buffer != NULL.
          strcpy(
              /*@-nullpass@*/ (char*)command_history_buffer /*@+nullpass@*/,
              (char*)new_command);
        }
        else
        {
          // Everything's fine: Enough space to store the new entry at the
          // beginning of the buffer.
          TRACE_LOG("Writing command to %p.\n", command_history_buffer);
          strcpy(
              /*@-nullpass@*/ (char*)command_history_buffer /*@+nullpass@*/,
              (char*)new_command); 
        }
      }
      else
      {
        // Nice: Enough space in buffer to append the new entry at the top
        // of it.
        TRACE_LOG("Enough space at top of buffer, writing %zu to %p.\n",
            input_size,
            command_history_buffer + end_of_used_space);

        command_history_entries[command_history_newest_entry]
          = end_of_used_space;

        TRACE_LOG("Writing command to %p.\n",
            command_history_buffer + end_of_used_space);

        strcpy(
            (char*)command_history_buffer + end_of_used_space,
            (char*)new_command); 

        command_history_highest_entry = command_history_newest_entry;
      }
    }
    else
    {
      TRACE_LOG("Writing in gap.\n");
      // In this case we're trying to write into the gap between the
      // newest entry below and the oldest entry above.

      current_index
        = command_history_entries[command_history_last_entry]
        + strlen((char*)command_history_buffer
            + command_history_entries[command_history_last_entry]) + 1;

      if (current_index + input_size
          >= command_history_entries[command_history_oldest_entry])
      {
        // Sadly, the new command is so long that we need to allocate
        // more space to keep the history alive.
        command_history_increase_buffer(input_size);
      }

      TRACE_LOG("Writing command to %p.\n",
          command_history_buffer + current_index);

      command_history_entries[command_history_newest_entry] = current_index;
      strcpy(
          (char*)command_history_buffer + current_index,
          (char*)new_command); 

      if (current_index
          > command_history_entries[command_history_highest_entry])
      {
        // In case we've just written behind the so far highest entry, we
        // have to update it.
        command_history_highest_entry = command_history_newest_entry;
      }
    }
  }

  TRACE_LOG("Lines in history: %d.\n", command_history_nof_entries);
  i = command_history_newest_entry;
  for (;;)
  {
    TRACE_LOG("%d(newest: %d / odlest:%d): \"%s\"\n", i+1,
        command_history_newest_entry,
        command_history_oldest_entry,
        command_history_buffer + command_history_entries[i]);
    
    if (i == command_history_oldest_entry)
    {
      break;
    }

    if (i == 0)
    {
      i = command_history_nof_entries - 1;
    }
    else
    {
      i--;
    }
  }
}

#endif // cmd_hist_c_INCLUDED

