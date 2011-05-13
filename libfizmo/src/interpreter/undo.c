
/* undo.c
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


#include <string.h>

#include "../tools/tracelog.h"
#include "../tools/types.h"
#include "undo.h"
#include "zpu.h"
#include "variable.h"
#include "fizmo.h"
#include "stack.h"
#include "config.h"


struct undo_frame
{
  uint8_t *dynamic_memory;
  uint8_t *pc;

  uint16_t *stack;
  int z_stack_size;
  int stack_words_from_active_routine;
  uint8_t number_of_locals_active;
};


static struct undo_frame* undo_frames[MAX_UNDO_STEPS];
static int undo_index = 0;


void opcode_save_undo(void)
{
  size_t dynamic_memory_size;
  struct undo_frame *new_undo_frame;
  int result;
  size_t stack_size_in_bytes;

  TRACE_LOG("Opcode: SAVE_UNDO.\n");

  if (MAX_UNDO_STEPS == 0)
  {
    result = 0;
  }
  else if ((new_undo_frame
        = (struct undo_frame*)malloc(sizeof(struct undo_frame))) == NULL)
  {
    result = 0;
  }
  else
  {
    dynamic_memory_size = (size_t)(
        active_z_story->dynamic_memory_end - z_mem + 1 );

    if ( (new_undo_frame->dynamic_memory = malloc(dynamic_memory_size))
        == NULL)
    {
      free(new_undo_frame);
      result = 0;
    }
    else
    {
      stack_size_in_bytes = (z_stack_index - z_stack) * sizeof(uint16_t);

      if ((new_undo_frame->stack
            = (uint16_t*)malloc(stack_size_in_bytes)) == NULL)
      {
        free(new_undo_frame->dynamic_memory);
        free(new_undo_frame);
        result = 0;
      }
      else
      {
        if (undo_index == MAX_UNDO_STEPS)
        {
          free(undo_frames[0]->stack);
          free(undo_frames[0]->dynamic_memory);
          free(undo_frames[0]);

          memmove(
              undo_frames,
              undo_frames + 1,
              sizeof(struct undo_frame*) * MAX_UNDO_STEPS - 1);

          undo_index--;
        }

        memcpy(
            new_undo_frame->stack,
            z_stack, // non-null when size > 0
            stack_size_in_bytes);

        memcpy(
            new_undo_frame->dynamic_memory,
            z_mem,
            dynamic_memory_size);

        // new_undo_frame->pc is not allocated, no free required.
        new_undo_frame->pc = pc;

        new_undo_frame->z_stack_size = z_stack_index - z_stack;
        new_undo_frame->stack_words_from_active_routine
          = stack_words_from_active_routine;
        new_undo_frame->number_of_locals_active
          = number_of_locals_active;

        undo_frames[undo_index++] = new_undo_frame;

        result = 1;
      }
    }
  }

  read_z_result_variable();
  set_variable(z_res_var, (uint16_t)result);
}

void opcode_restore_undo(void)
{
  int result;
  size_t dynamic_memory_size;
  struct undo_frame *frame_to_restore;

  TRACE_LOG("Opcode: RESTORE_UNDO.\n");

  if (undo_index > 0)
  {
    undo_index--;

    frame_to_restore = undo_frames[undo_index];

    dynamic_memory_size = (size_t)(
        active_z_story->dynamic_memory_end - z_mem + 1 );

    ensure_z_stack_size(frame_to_restore->z_stack_size);

    pc =  frame_to_restore->pc;
    current_z_stack_size = frame_to_restore->z_stack_size;
    z_stack_index = z_stack + frame_to_restore->z_stack_size;
    stack_words_from_active_routine
      = frame_to_restore->stack_words_from_active_routine;
    number_of_locals_active
      = frame_to_restore->number_of_locals_active;
    local_variable_storage_index
      = z_stack_index
      - stack_words_from_active_routine
      - number_of_locals_active;

    memcpy(
        z_stack, // non-null when size > 0
        frame_to_restore->stack,
        current_z_stack_size * sizeof(uint16_t));

    memcpy(
        z_mem,
        frame_to_restore->dynamic_memory,
        dynamic_memory_size);

    free(frame_to_restore->stack);
    free(frame_to_restore->dynamic_memory);
    free(frame_to_restore);

    write_interpreter_info_into_header();

    result = 2;
  }
  else
  {
    result = 0;
  }

  read_z_result_variable();
  set_variable(z_res_var, (uint16_t)result);
}


size_t get_allocated_undo_memory_size(void)
{
  int i = 0;
  size_t dynamic_memory_size = (size_t)(
      active_z_story->dynamic_memory_end - z_mem + 1 );
  size_t result
    = undo_index * (sizeof(struct undo_frame) + dynamic_memory_size);

  while (i < undo_index)
    result += undo_frames[i++]->z_stack_size * sizeof(uint16_t);

  return result;
}


