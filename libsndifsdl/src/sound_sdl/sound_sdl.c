
/* sound_sdl.c
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


#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 1

#ifndef sound_sdl_c_INCLUDED
#define sound_sdl_c_INCLUDED


#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_main.h>
#include <SDL_audio.h>
#include <SDL_thread.h>

#ifdef ENABLE_AIFF_FOR_SOUND_SDL
#include "sndfile.h"
#endif // ENABLE_AIFF_FOR_SOUND_SDL

#include "sound_interface/sound_interface.h"
#include "tools/tracelog.h"
#include "tools/unused.h"
#include "tools/filesys.h"
#include "interpreter/zpu.h"
#include "interpreter/config.h"
#include "interpreter/fizmo.h"
#include "interpreter/streams.h"
#include "interpreter/blorb.h"
#include "sound_sdl.h"

// Including timezone on linux doesn't work?
/*
#ifdef ENABLE_TRACING
#include <time.h>
#include <sys/time.h>
#endif
*/

//#define TONE_880_HZ_CYCLE_DURATION 250
#define TONE_880_HZ_SAMPLE_SIZE 400
#define TONE_880_HZ_REPEATS 8

//#define TONE_330_HZ_CYCLE_DURATION 250
#define TONE_330_HZ_SAMPLE_SIZE 800
#define TONE_330_HZ_REPEATS 6

static char *sdl_interface_name = "libsdlsound";
static char *sdl_interface_version = "0.7.0";

struct sound_effect
{
  Uint8 *data;
  int data_len;
  bool is_internal_effect;
  int nof_channels;
  int format;
  int freq;
  int nof_repeats;
  int sound_volume;
  uint16_t routine_after_playback;
};

static uint16_t *routine_stack = NULL;
static int routine_stack_top_element = -1;
static int routine_stack_size = 0;

static struct sound_effect **sdl_sound_effect_fifo = NULL;
static int sound_effect_top_element = -1;
static int sound_effect_fifo_size = 0;
static struct sound_effect *current_effect;
//SDL_mutex *effect_stack_mutex;

static bool is_audio_open = false;
static bool sound_output_active = false;
static SDL_mutex *sound_output_active_mutex;

static bool sound_init_performed = false;

static SDL_Thread *effect_thread = NULL;
static bool stop_effect_thread = false;

static char *sdl_file_prefix = NULL;
static char *sdl_file_prefix_lower = NULL;
static char *sdl_file_prefix_upper = NULL;
static char *sdl_directory_name = NULL;
//long current_sample_size;
static long current_sample_index;
//Uint8 *sample_data;
//Uint8 *sample_buffer = NULL;
//Uint32 sample_buffer_size = 0;
static char *sdl_snd_filename;
//int number_of_repeats;
static bool read_has_occurred = true;
//static bool stop_current_sound = false;
static bool flush_sound_effect_stack = false;
//Mix_Chunk *sound_effect_chunk = NULL;
//uint16_t routine_after_playback = 0;
static bool force_8bit_sound = false;
static char *config_option_names[] = { "force-8bit-sound", NULL } ;

static SDL_TimerID sdl_finish_timer = NULL;


Uint8 tone880hz[] = {
  0xfa, 0xf7, 0xbe, 0x68, 0x1d, 0x00, 0x1d, 0x68, 0xbe, 0xf7, 0xfa, 0xc5,
  0x70, 0x23, 0x00, 0x18, 0x60, 0xb6, 0xf4, 0xfc, 0xcb, 0x78, 0x29, 0x01,
  0x14, 0x59, 0xaf, 0xf0, 0xfe, 0xd2, 0x80, 0x2e, 0x02, 0x10, 0x51, 0xa8,
  0xec, 0xff, 0xd8, 0x88, 0x35, 0x04, 0x0c, 0x4a, 0xa0, 0xe7, 0xff, 0xde,
  0x90, 0x3c, 0x07, 0x09, 0x43, 0x98, 0xe2, 0xff, 0xe3, 0x98, 0x42, 0x09,
  0x06, 0x3b, 0x90, 0xdd, 0xff, 0xe8, 0xa0, 0x49, 0x0c, 0x04, 0x35, 0x88,
  0xd7, 0xff, 0xec, 0xa7, 0x51, 0x10, 0x02, 0x2e, 0x80, 0xd2, 0xfd, 0xf0,
  0xb0, 0x59, 0x14, 0x01, 0x28, 0x78, 0xcb, 0xfc, 0xf4, 0xb6, 0x60, 0x19,
  0x00, 0x23, 0x70, 0xc5, 0xfa, 0xf7, 0xbe, 0x68, 0x1d, 0x00, 0x1d, 0x68,
  0xbe, 0xf7, 0xfa, 0xc4, 0x70, 0x23, 0x00, 0x18, 0x60, 0xb6, 0xf3, 0xfc,
  0xcb, 0x78, 0x29, 0x01, 0x14, 0x59, 0xaf, 0xf0, 0xfe, 0xd2, 0x80, 0x2e,
  0x02, 0x10, 0x51, 0xa7, 0xec, 0xff, 0xd8, 0x88, 0x35, 0x04, 0x0c, 0x4a,
  0xa0, 0xe7, 0xff, 0xdd, 0x90, 0x3b, 0x06, 0x09, 0x42, 0x98, 0xe3, 0xff,
  0xe3, 0x98, 0x42, 0x09, 0x07, 0x3c, 0x90, 0xdd, 0xff, 0xe8, 0xa0, 0x4a,
  0x0c, 0x04, 0x35, 0x88, 0xd8, 0xff, 0xec, 0xa8, 0x51, 0x10, 0x02, 0x2e,
  0x80, 0xd1, 0xfe, 0xf0, 0xaf, 0x58, 0x14, 0x01, 0x28, 0x78, 0xcb, 0xfc,
  0xf4, 0xb6, 0x60, 0x18, 0x00, 0x23, 0x70, 0xc5, 0xfa, 0xf7, 0xbd, 0x68,
  0x1d, 0x00, 0x1d, 0x68, 0xbe, 0xf7, 0xfa, 0xc5, 0x70, 0x23, 0x00, 0x18,
  0x60, 0xb7, 0xf4, 0xfc, 0xcb, 0x78, 0x28, 0x01, 0x14, 0x58, 0xaf, 0xf0,
  0xfe, 0xd2, 0x80, 0x2e, 0x03, 0x10, 0x50, 0xa7, 0xec, 0xff, 0xd8, 0x88,
  0x35, 0x04, 0x0c, 0x49, 0xa0, 0xe7, 0xff, 0xdd, 0x90, 0x3b, 0x06, 0x09,
  0x43, 0x98, 0xe3, 0xff, 0xe3, 0x98, 0x42, 0x09, 0x06, 0x3b, 0x90, 0xdd,
  0xff, 0xe8, 0x9f, 0x4a, 0x0c, 0x04, 0x35, 0x88, 0xd8, 0xff, 0xec, 0xa7,
  0x51, 0x10, 0x02, 0x2f, 0x80, 0xd1, 0xfe, 0xf0, 0xaf, 0x58, 0x14, 0x01,
  0x29, 0x78, 0xcc, 0xfc, 0xf4, 0xb6, 0x60, 0x19, 0x00, 0x23, 0x70, 0xc5,
  0xfa, 0xf7, 0xbe, 0x68, 0x1d, 0x00, 0x1d, 0x68, 0xbe, 0xf7, 0xf9, 0xc5,
  0x70, 0x22, 0x00, 0x19, 0x60, 0xb6, 0xf4, 0xfc, 0xcb, 0x78, 0x29, 0x01,
  0x14, 0x58, 0xaf, 0xf0, 0xfd, 0xd2, 0x80, 0x2e, 0x03, 0x10, 0x51, 0xa8,
  0xec, 0xff, 0xd7, 0x88, 0x35, 0x04, 0x0c, 0x4a, 0xa0, 0xe7, 0xff, 0xdd,
  0x90, 0x3b, 0x07, 0x09, 0x42, 0x98, 0xe3, 0xff, 0xe2, 0x98, 0x42, 0x09,
  0x06, 0x3b, 0x90, 0xde, 0xff, 0xe7, 0xa0, 0x4a, 0x0c, 0x04, 0x35, 0x88,
  0xd8, 0xff, 0xec, 0xa7, 0x51, 0x10, 0x03, 0x2f, 0x80, 0xd2, 0xfe, 0xf0,
  0xaf, 0x58, 0x14, 0x01, 0x28, 0x78, 0xcb, 0xfc, 0xf4, 0xb7, 0x60, 0x18,
  0x00, 0x23, 0x70, 0xc5 };

Uint8 tone330hz[] = {
  0xee, 0xfb, 0xff, 0xfc, 0xf0, 0xdd, 0xc3, 0xa5, 0x84, 0x63, 0x44, 0x29,
  0x14, 0x06, 0x00, 0x03, 0x0e, 0x21, 0x3a, 0x57, 0x78, 0x99, 0xb8, 0xd4,
  0xea, 0xf9, 0xff, 0xfe, 0xf4, 0xe2, 0xca, 0xac, 0x8c, 0x6b, 0x4b, 0x2f,
  0x18, 0x09, 0x01, 0x01, 0x0b, 0x1c, 0x33, 0x50, 0x70, 0x91, 0xb1, 0xce,
  0xe5, 0xf6, 0xff, 0xff, 0xf7, 0xe7, 0xd0, 0xb4, 0x94, 0x73, 0x53, 0x36,
  0x1d, 0x0c, 0x02, 0x01, 0x08, 0x17, 0x2d, 0x48, 0x68, 0x89, 0xaa, 0xc7,
  0xe0, 0xf2, 0xfe, 0xff, 0xfa, 0xeb, 0xd6, 0xbb, 0x9c, 0x7b, 0x5a, 0x3c,
  0x22, 0x10, 0x03, 0x00, 0x05, 0x12, 0x27, 0x41, 0x60, 0x81, 0xa2, 0xc0,
  0xda, 0xef, 0xfb, 0xff, 0xfc, 0xf0, 0xdc, 0xc2, 0xa4, 0x83, 0x62, 0x44,
  0x28, 0x14, 0x06, 0x00, 0x03, 0x0f, 0x21, 0x3a, 0x58, 0x79, 0x9a, 0xb9,
  0xd5, 0xea, 0xf9, 0xff, 0xfe, 0xf3, 0xe1, 0xc9, 0xab, 0x8b, 0x6a, 0x4a,
  0x2e, 0x18, 0x08, 0x01, 0x01, 0x0b, 0x1c, 0x34, 0x51, 0x71, 0x92, 0xb2,
  0xce, 0xe6, 0xf6, 0xff, 0xff, 0xf6, 0xe7, 0xcf, 0xb3, 0x93, 0x72, 0x52,
  0x34, 0x1d, 0x0b, 0x02, 0x01, 0x08, 0x18, 0x2d, 0x49, 0x69, 0x8a, 0xaa,
  0xc8, 0xe1, 0xf3, 0xfe, 0xff, 0xf9, 0xeb, 0xd6, 0xba, 0x9b, 0x7a, 0x59,
  0x3c, 0x22, 0x0f, 0x03, 0x00, 0x05, 0x13, 0x28, 0x42, 0x61, 0x82, 0xa3,
  0xc1, 0xdb, 0xef, 0xfc, 0xff, 0xfb, 0xef, 0xdb, 0xc1, 0xa3, 0x82, 0x61,
  0x42, 0x28, 0x13, 0x05, 0x00, 0x03, 0x0f, 0x22, 0x3b, 0x5a, 0x7a, 0x9b,
  0xba, 0xd5, 0xeb, 0xfa, 0xff, 0xfe, 0xf3, 0xe0, 0xc8, 0xaa, 0x8a, 0x69,
  0x4a, 0x2d, 0x17, 0x08, 0x01, 0x02, 0x0c, 0x1d, 0x35, 0x52, 0x72, 0x93,
  0xb3, 0xcf, 0xe6, 0xf6, 0xff, 0xff, 0xf6, 0xe6, 0xce, 0xb2, 0x92, 0x71,
  0x51, 0x34, 0x1c, 0x0b, 0x02, 0x01, 0x09, 0x18, 0x2f, 0x4b, 0x6a, 0x8b,
  0xab, 0xc9, 0xe2, 0xf4, 0xfe, 0xff, 0xf9, 0xeb, 0xd5, 0xb9, 0x9a, 0x79,
  0x58, 0x3a, 0x21, 0x0e, 0x03, 0x00, 0x06, 0x13, 0x29, 0x43, 0x62, 0x83,
  0xa4, 0xc2, 0xdc, 0xf0, 0xfc, 0xff, 0xfc, 0xee, 0xda, 0xc1, 0xa2, 0x81,
  0x60, 0x42, 0x27, 0x12, 0x05, 0x00, 0x03, 0x0f, 0x22, 0x3c, 0x5a, 0x7b,
  0x9c, 0xbb, 0xd6, 0xeb, 0xfa, 0xff, 0xfd, 0xf3, 0xe0, 0xc7, 0xa9, 0x89,
  0x68, 0x49, 0x2d, 0x17, 0x07, 0x00, 0x02, 0x0c, 0x1d, 0x36, 0x53, 0x73,
  0x94, 0xb4, 0xd0, 0xe7, 0xf7, 0xff, 0xff, 0xf6, 0xe5, 0xce, 0xb1, 0x91,
  0x70, 0x50, 0x33, 0x1c, 0x0a, 0x02, 0x01, 0x09, 0x18, 0x2f, 0x4b, 0x6b,
  0x8c, 0xac, 0xc9, 0xe2, 0xf4, 0xfe, 0xff, 0xf9, 0xea, 0xd4, 0xb8, 0x99,
  0x78, 0x58, 0x3a, 0x21, 0x0e, 0x03, 0x00, 0x06, 0x14, 0x29, 0x44, 0x63,
  0x84, 0xa4, 0xc3, 0xdd, 0xf0, 0xfc, 0xff, 0xfb, 0xee, 0xda, 0xbf, 0xa1,
  0x80, 0x5f, 0x40, 0x26, 0x12, 0x05, 0x00, 0x04, 0x10, 0x24, 0x3d, 0x5c,
  0x7c, 0x9d, 0xbc, 0xd7, 0xec, 0xfa, 0xff, 0xfd, 0xf2, 0xdf, 0xc7, 0xa9,
  0x88, 0x67, 0x47, 0x2c, 0x16, 0x07, 0x01, 0x02, 0x0c, 0x1e, 0x36, 0x54,
  0x74, 0x95, 0xb5, 0xd1, 0xe8, 0xf7, 0xff, 0xff, 0xf5, 0xe5, 0xcd, 0xb0,
  0x90, 0x6f, 0x4f, 0x32, 0x1b, 0x0a, 0x01, 0x01, 0x09, 0x19, 0x30, 0x4d,
  0x6c, 0x8d, 0xad, 0xcb, 0xe3, 0xf4, 0xfe, 0xff, 0xf8, 0xe9, 0xd3, 0xb7,
  0x98, 0x77, 0x56, 0x39, 0x20, 0x0e, 0x03, 0x00, 0x06, 0x15, 0x2a, 0x45,
  0x64, 0x85, 0xa6, 0xc3, 0xdd, 0xf1, 0xfc, 0xff, 0xfb, 0xed, 0xd9, 0xbf,
  0xa0, 0x7f, 0x5e, 0x3f, 0x25, 0x11, 0x04, 0x00, 0x04, 0x10, 0x24, 0x3e,
  0x5c, 0x7d, 0x9e, 0xbd, 0xd8, 0xed, 0xfa, 0xff, 0xfd, 0xf1, 0xdf, 0xc5,
  0xa8, 0x87, 0x66, 0x46, 0x2c, 0x16, 0x07, 0x00, 0x02, 0x0c, 0x1e, 0x37,
  0x55, 0x75, 0x96, 0xb6, 0xd2, 0xe8, 0xf8, 0xff, 0xfe, 0xf5, 0xe4, 0xcd,
  0xaf, 0x90, 0x6e, 0x4e, 0x31, 0x1a, 0x0a, 0x01, 0x01, 0x09, 0x1a, 0x31,
  0x4d, 0x6d, 0x8e, 0xae, 0xcb, 0xe3, 0xf5, 0xfe, 0xff, 0xf8, 0xe9, 0xd2,
  0xb7, 0x97, 0x76, 0x56, 0x38, 0x1f, 0x0d, 0x02, 0x00, 0x07, 0x15, 0x2b,
  0x46, 0x65, 0x86, 0xa6, 0xc5, 0xde, 0xf1, 0xfd, 0xff, 0xfb, 0xed, 0xd8,
  0xbe, 0x9f, 0x7e, 0x5d, 0x3f, 0x24, 0x11, 0x04, 0x00, 0x04, 0x11, 0x25,
  0x3f, 0x5d, 0x7e, 0x9e, 0xbd, 0xd8, 0xed, 0xfb, 0xff, 0xfc, 0xf1, 0xde,
  0xc5, 0xa7, 0x86, 0x65, 0x46, 0x2b, 0x15, 0x06, 0x00, 0x02, 0x0d, 0x1f,
  0x38, 0x56, 0x76, 0x97, 0xb7, 0xd2, 0xe9, 0xf8, 0xff, 0xfe, 0xf4, 0xe3,
  0xcb, 0xae, 0x8e, 0x6d, 0x4d, 0x31, 0x1a, 0x0a, 0x01, 0x01, 0x0a, 0x1a,
  0x32, 0x4e, 0x6e, 0x8f, 0xaf, 0xcc, 0xe4, 0xf5, 0xfe, 0xff, 0xf7, 0xe8,
  0xd2, 0xb6, 0x96, 0x75, 0x55, 0x37, 0x1e, 0x0d, 0x02, 0x00, 0x07, 0x16,
  0x2b, 0x47, 0x66, 0x87, 0xa8, 0xc5, 0xdf, 0xf2, 0xfd, 0xff, 0xfa, 0xed,
  0xd7, 0xbd, 0x9e, 0x7d, 0x5c, 0x3e, 0x24, 0x11, 0x04, 0x00, 0x04, 0x11,
  0x25, 0x40, 0x5e, 0x7f, 0xa0, 0xbf, 0xd9, 0xee, 0xfb, 0xff, 0xfd, 0xf1,
  0xdd, 0xc4, 0xa6, 0x85, 0x64, 0x45, 0x2a, 0x14, 0x06, 0x00, 0x03, 0x0e,
  0x20, 0x39, 0x57, 0x77, 0x98, 0xb7, 0xd3, 0xe9, 0xf9, 0xff, 0xfe, 0xf4,
  0xe2, 0xca, 0xad, 0x8d, 0x6c, 0x4c, 0x30, 0x19, 0x09, 0x01, 0x01, 0x0a,
  0x1b, 0x32, 0x4f, 0x6f, 0x90, 0xb0, 0xcd, 0xe4, 0xf5, 0xff, 0xff, 0xf8,
  0xe8, 0xd1, 0xb5, 0x95, 0x74, 0x54, 0x37, 0x1e, 0x0c, 0x02, 0x01, 0x08,
  0x16, 0x2c, 0x48, 0x67, 0x88, 0xa9, 0xc6, 0xe0, 0xf2, 0xfd, 0xff, 0xfa,
  0xec, 0xd7, 0xbc, 0x9d, 0x7c, 0x5b, 0x3d, 0x23, 0x10, 0x04, 0x00, 0x05,
  0x12, 0x27, 0x40, 0x5f, 0x80, 0xa1, 0xbf, 0xda };



static void clear_all_effects_from_stack()
{
  int i;

  for (i=sound_effect_top_element; i>=0; i--)
    if (sdl_sound_effect_fifo[i]->is_internal_effect == false)
      free(sdl_sound_effect_fifo[i]->data);

  sound_effect_top_element = -1;
}


static void clear_current_effect_from_stack()
{
  SDL_TimerID timer_to_terminate;

  TRACE_LOG("\n\n[sound]Clearing current effect from stack.\n\n");

  timer_to_terminate = sdl_finish_timer;
  if (timer_to_terminate != NULL)
  {
    SDL_RemoveTimer(timer_to_terminate);
  }

  if (current_effect->is_internal_effect == false)
    free(current_effect->data);

  memmove(
      sdl_sound_effect_fifo,
      sdl_sound_effect_fifo + 1,
      sizeof(struct sound_effect*) * sound_effect_top_element);

  sound_effect_top_element--;
}


static Uint32 sdl_effect_finished(Uint32 UNUSED(interval), void* UNUSED(param))
{
  TRACE_LOG("\n\n[sound]effect finished on timer.\n\n");

  SDL_mutexP(sound_output_active_mutex);
  sdl_finish_timer = NULL;

  TRACE_LOG("\n\n[sound]effect finished(%d).\n\n", sound_effect_top_element);
  SDL_PauseAudio(1);

  if (current_effect->routine_after_playback != 0)
  {
    routine_stack_top_element++;
    if (routine_stack_top_element == routine_stack_size)
    {
      routine_stack_size += 4;
      routine_stack = (uint16_t*)fizmo_realloc(
          routine_stack, routine_stack_size * sizeof(uint16_t));
    }
    routine_stack[routine_stack_top_element]
      = current_effect->routine_after_playback;

    TRACE_LOG("[sound]routine_stack[%d] = %d.\n", routine_stack_top_element,
        current_effect->routine_after_playback);
    }

  clear_current_effect_from_stack();

  sound_output_active = false;

  SDL_mutexV(sound_output_active_mutex);

  // We can't simply start a new sound directly from here, since a new
  // sound may have a new sample rate and setting a new sample rate
  // requires a SDL_OpenAudio call, which requires a SDL_CloseAudio
  // beforehand, which can't be issued from inside this callback routine.
  TRACE_LOG("\n\n[sound]alarm-callback call quit (effect finished).\n\n");

  SDL_mutexV(sound_output_active_mutex);

  return 0;
}


void mixaudio(void *UNUSED(unused), Uint8 *stream, int len)
{
  Uint32 interval;
  int size;

  TRACE_LOG("\n\n[sound]mixaudio call.\n\n");

  while ( (current_effect->nof_repeats != 0) && (len > 0) )
  {
    size = current_effect->data_len - current_sample_index;

    TRACE_LOG("\n\n[sound]repeats: %d, len:%d, size:%d.\n\n",
        current_effect->nof_repeats, len, size);

    if (len < size)
      size = len;

    SDL_MixAudio(
        stream,
        current_effect->data + current_sample_index,
        size,
        current_effect->sound_volume);

    stream += size;
    len -= size;
    current_sample_index += size;

    if (current_sample_index == current_effect->data_len)
    {
      current_sample_index = 0;

      if (current_effect->nof_repeats >= 0)
      {
        if (flush_sound_effect_stack == true)
        {
          SDL_mutexP(sound_output_active_mutex);

          if (sdl_finish_timer == NULL)
          {
            // Wait to complete a full cycle.
            interval
              = ((double)size / (double)current_effect->freq)
              * 1000
              + 1000; //timing-safety
              // + 100; //timing-safety
            TRACE_LOG("\n\n[sound]interval(%d/%d):%u ms\n",
                size, current_effect->freq, interval);
            sdl_finish_timer = SDL_AddTimer(
                interval, &sdl_effect_finished, NULL);
          }
          SDL_mutexV(sound_output_active_mutex);
        }

        current_effect->nof_repeats--;

        if (current_effect->nof_repeats == 0)
        {
          len = 0;
          break;
        }
      }
    }
  }

  TRACE_LOG("\n\n[sound]mixaudio call done.\n\n");
}


void sdl_init_sound()
{
  int len;
  char *slashpos, *dotpos;
  int i;

  TRACE_LOG("[sound]sdl_init_sound() called.\n");

  if (sound_init_performed == true)
    return;

  TRACE_LOG("[sound]Initializing sound.n");

  if ((slashpos = strrchr(active_z_story->absolute_file_name, '/')) != NULL)
  {
    len = slashpos - active_z_story->absolute_file_name;
    sdl_directory_name = (char*)fizmo_malloc(len + 1);
    memcpy(sdl_directory_name, active_z_story->absolute_file_name, len);
    sdl_directory_name[len] = '\0';

    if ((dotpos = strchr(slashpos + 1, '.')) != NULL)
      len = dotpos - slashpos;
    else
      len = strlen(active_z_story->absolute_file_name) - len - 1;

    if (len > 6)
      len = 6;

    sdl_file_prefix = (char*)fizmo_malloc(len + 1);
    sdl_file_prefix_lower = (char*)fizmo_malloc(len + 1);
    sdl_file_prefix_upper = (char*)fizmo_malloc(len + 1);

    memcpy(sdl_file_prefix, slashpos + 1, len);
    for (i=0; i<len; i++)
    {
      sdl_file_prefix_lower[i] = (char)tolower(sdl_file_prefix[i]);
      sdl_file_prefix_upper[i] = (char)toupper(sdl_file_prefix[i]);
    }
    sdl_file_prefix_lower[i] = '\0';
    sdl_file_prefix_upper[i] = '\0';

    TRACE_LOG("%s / %s\n", sdl_directory_name, sdl_file_prefix);

    sdl_snd_filename
      = (char*)fizmo_malloc(strlen(sdl_directory_name) + len + 7);
  }
  else
  {
    sdl_directory_name = NULL;
    sdl_file_prefix = NULL;
  }

  TRACE_LOG("[sound]Invoking SDL_Init.\n");

  SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO);

  sound_output_active_mutex = SDL_CreateMutex();

  sound_init_performed = true;
}


void sdl_close_sound()
{
  int thread_status;

  TRACE_LOG("sdl_close_sound invoked.\n");

  if (sound_init_performed == false)
  {
    TRACE_LOG("No init performed, returning.\n");
    return;
  }

  stop_effect_thread = true;
  SDL_WaitThread(effect_thread, &thread_status);

  SDL_mutexP(sound_output_active_mutex);

  TRACE_LOG("Pausing and closing audio.\n");

  SDL_PauseAudio(1);
  SDL_CloseAudio();
  errno = 0;
  sound_output_active = false;
  
  clear_all_effects_from_stack();

  free(sdl_sound_effect_fifo);
  sdl_sound_effect_fifo = NULL;
  sound_effect_fifo_size = 0;

  sound_init_performed = false;
  SDL_mutexV(sound_output_active_mutex);
  SDL_DestroyMutex(sound_output_active_mutex);
  SDL_Quit();
}


void sdl_prepare_sound(int UNUSED(sound_nr), int UNUSED(volume),
    int UNUSED(repeats))
{
  if (sound_init_performed == false)
    return;
}


// This method has to be called with locked semaphore.
static void start_next_effect()
{
  SDL_AudioSpec fmt;
  Uint32 interval;

  SDL_PauseAudio(1);

  if (is_audio_open == true)
  {
    SDL_CloseAudio();
    errno = 0;
    is_audio_open = false;
  }

  if (sound_effect_top_element < 0)
    return;
  else if (sound_effect_top_element == 0)
  {
    flush_sound_effect_stack = false;
    read_has_occurred = false;
  }

  current_sample_index = 0;
  current_effect = *sdl_sound_effect_fifo;

  TRACE_LOG("\n\n[sound]new effect loaded (%d).\n", sound_effect_top_element);

  fmt.format = current_effect->format;
  fmt.samples = 8192;
  fmt.callback = &mixaudio;
  fmt.userdata = NULL;
  fmt.freq = current_effect->freq;
  fmt.channels = current_effect->nof_channels;

  TRACE_LOG("[sound]New effect, freq: %d.\n", current_effect->freq);

  TRACE_LOG("[sound]errno:%d\n", errno);
  if (SDL_OpenAudio(&fmt, NULL) < 0 )
  {
    TRACE_LOG("[sound]Unable to open audio: %s\n", SDL_GetError());
    return;
  }
  TRACE_LOG("[sound]errno:%d\n", errno);
  errno = 0;
  is_audio_open = true;

  //routine_after_playback = routine;
  sound_output_active = true;

  if (current_effect->nof_repeats > 0)
  {
    interval
      = ((double)current_effect->data_len / (double)current_effect->freq)
      * current_effect->nof_repeats
    * 1000
    + 100; //timing-safety
    TRACE_LOG("\n\n[sound]interval(%d/%d):%u ms\n",
        current_effect->data_len, current_effect->freq, interval);
    sdl_finish_timer = SDL_AddTimer(interval, &sdl_effect_finished, NULL);
  }
  SDL_PauseAudio(0);
}


int effect_thread_routine(void* UNUSED(parameter))
{
  /*
#ifdef ENABLE_TRACING
  time_t rawtime;
  struct timeval tv;
  struct timezone tz;
  struct tm *tm;
#endif
  */

  for (;;)
  {
    /*
#ifdef ENABLE_TRACING
    time ( &rawtime );
    gettimeofday(&tv, &tz);
    tm=localtime(&tv.tv_sec);

    TRACE_LOG(
        "\n\n[sound](%s,%ld)effect_thread_routine run (%d:%02d:%02d %d)\n\n",
        ctime (&rawtime), rawtime, tm->tm_hour, tm->tm_min,
        tm->tm_sec, tv.tv_usec);
#endif
    */

    SDL_Delay(25);

    SDL_mutexP(sound_output_active_mutex);

    if (
        (stop_effect_thread == true)
        ||
        ( (sound_output_active == false) && (sound_effect_top_element < 0) )
       )
    {
      effect_thread = NULL;
      SDL_mutexV(sound_output_active_mutex);
      TRACE_LOG("\n\n[sound]effect_thread_routine quit.\n\n");
      return 0;
    }

    if (sound_output_active == false)
    {
      // In case the sound has stopped playing, start the next one (if
      // there's at least one left on the stack) or quit this thread.

      start_next_effect();
    }

    SDL_mutexV(sound_output_active_mutex);
  }
}


void sdl_play_sound(int sound_nr, int volume, int repeats, uint16_t routine)
{
  struct sound_effect *effect;
  //int delay;
  int nof_repeats_from_file;
  z_file *in;
  int src_len, base_note, frequency;
#ifdef ENABLE_AIFF_FOR_SOUND_SDL
  SF_INFO sfinfo;
  SNDFILE *sndfile;
  int index, len;
  int i;
  bool use_8bit_sound;
  //struct z_story_blorb_sound *sound_blorb_index;
  long sound_blorb_index;
  int fd;
  short shortbuf[256];
  int v3_sound_loops;
#endif // ENABLE_AIFF_FOR_SOUND_SDL
  /*
#ifdef ENABLE_TRACING
  time_t rawtime;
  struct timeval tv;
  struct timezone tz;
  struct tm *tm;
#endif
  */

  if (sound_init_performed == false)
    return;

  effect = (struct sound_effect*)fizmo_malloc(sizeof(struct sound_effect));

  if (sound_nr <= 2)
  {
    effect->is_internal_effect = true;
    effect->format = AUDIO_U8;
    effect->nof_channels = 1;
    effect->freq = 8000;

    if (sound_nr == 1)
    {
      effect->data = tone880hz;
      effect->data_len = TONE_880_HZ_SAMPLE_SIZE;
      //delay = TONE_880_HZ_CYCLE_DURATION;
      effect->nof_repeats = TONE_880_HZ_REPEATS;
    }
    else if (sound_nr == 2)
    {
      effect->data = tone330hz;
      effect->data_len = TONE_330_HZ_SAMPLE_SIZE;
      //delay = TONE_330_HZ_CYCLE_DURATION;
      effect->nof_repeats = TONE_330_HZ_REPEATS;
    }
  }
  else
  {
#ifdef ENABLE_AIFF_FOR_SOUND_SDL
    TRACE_LOG("Trying to find blorb sound number %d.\n", sound_nr);
    if ((sound_blorb_index = active_blorb_interface->get_blorb_offset(
            active_z_story->blorb_map, Z_BLORB_TYPE_SOUND, sound_nr)) != -1)
    {
      fd = fsi->get_fileno(active_z_story->blorb_file);
      TRACE_LOG("Seeking pos %x\n", sound_blorb_index-8);
      lseek(fd, sound_blorb_index-8, SEEK_SET);

      memset(&sfinfo, 0, sizeof (sfinfo));

      if ((sndfile = sf_open_fd(
              fd,
              SFM_READ,
              &sfinfo,
              SF_FALSE)) != NULL)
      {
        TRACE_LOG("open ok.\n");

        if (sfinfo.channels > 2)
        {
          free(effect);
          sf_close(sndfile);
          return;
        }

        if (
            ((sfinfo.format & SF_FORMAT_PCM_S8) != 0)
            ||
            ((sfinfo.format & SF_FORMAT_PCM_U8) != 0)
            ||
            ((sfinfo.format & SF_FORMAT_DPCM_8) != 0)
           )
          use_8bit_sound = true;
        else
          use_8bit_sound
            = strcmp(get_configuration_value("force-8bit-sound"),
                config_true_value) == 0
            ? true
            : false;

        TRACE_LOG("8-bit sound: %d\n", use_8bit_sound);

        effect->is_internal_effect = false;
        effect->nof_channels = sfinfo.channels;
        effect->freq = sfinfo.samplerate;
        effect->data_len = sfinfo.frames;

        if (use_8bit_sound == true)
        {
          effect->format = AUDIO_S8;
          effect->data = fizmo_malloc(sfinfo.frames * sfinfo.channels);
        }
        else
        {
          effect->format = AUDIO_S16;
          effect->data_len *= 2;
          effect->data = fizmo_malloc(sfinfo.frames * sfinfo.channels * 2);
        }

        if (ver < 5)
        {
          v3_sound_loops = get_v3_sound_loops_from_blorb_map(
              active_z_story->blorb_map, sound_nr);

          effect->nof_repeats
            = v3_sound_loops >= 1
            ? v3_sound_loops
            : -1;
        }
        else
        {
          effect->nof_repeats = repeats;
        }

        TRACE_LOG("[sound]channels: %d, freq: %d, samples: %ld, repeats:%d.\n",
            sfinfo.channels, sfinfo.samplerate, (long int)sfinfo.frames,
            effect->nof_repeats);

        index = 0;
        while ((len = sf_readf_short(
                sndfile,
                shortbuf,
                128 * sfinfo.channels)) > 0)
        {
          for (i=0; i<len * sfinfo.channels; i++)
          {
            //TRACE_LOG("%d: %d\n", index, shortbuf[i] / 256 + 128);

            if (use_8bit_sound == true)
              effect->data[index++] = shortbuf[i] >> 8;
            else
            {
              effect->data[index++] = shortbuf[i] & 0xff;
              effect->data[index++] = shortbuf[i] >> 8;
            }
          }
        }

        sf_close(sndfile);
      }
      else
      {
        TRACE_LOG("retval: %s\n", sf_strerror(NULL));
        return;
      }
    }
#endif // ENABLE_AIFF_FOR_SOUND_SDL
    else if (
        (sdl_directory_name != NULL)
        &&
        (sdl_file_prefix != NULL)
        )
    {
      // Try ".SND" effects
      sprintf(sdl_snd_filename, "%s/%s%02d.snd",
          sdl_directory_name, sdl_file_prefix, sound_nr);

      TRACE_LOG("[sound]Trying to open sound file \"%s\".\n", sdl_snd_filename);
      if ((in = fsi->openfile(sdl_snd_filename, FILETYPE_DATA, FILEACCESS_READ))
          == NULL)
      {
        sprintf(sdl_snd_filename, "%s/%s%02d.SND",
            sdl_directory_name, sdl_file_prefix, sound_nr);

        TRACE_LOG("[sound]Trying to open sound file \"%s\".\n",
            sdl_snd_filename);
        if ((in = fsi->openfile(
                sdl_snd_filename, FILETYPE_DATA, FILEACCESS_READ)) == NULL)
        {
          sprintf(sdl_snd_filename, "%s/%s%02d.SND",
              sdl_directory_name, sdl_file_prefix_upper, sound_nr);

          TRACE_LOG("[sound]Trying to open sound file \"%s\".\n",
              sdl_snd_filename);

          if ((in = fsi->openfile(
                  sdl_snd_filename, FILETYPE_DATA, FILEACCESS_READ)) == NULL)
          {
            sprintf(sdl_snd_filename, "%s/%s%02d.snd",
                sdl_directory_name, sdl_file_prefix_lower, sound_nr);

            TRACE_LOG("[sound]Trying to open sound file \"%s\".\n",
                sdl_snd_filename);

            if ((in = fsi->openfile(
                    sdl_snd_filename, FILETYPE_DATA, FILEACCESS_READ)) == NULL)
            {
              free(effect);
              return;
            }
          }
        }
      }

      src_len = ((int)fsi->getchar(in) << 8) | fsi->getchar(in);
      nof_repeats_from_file = (int)fsi->getchar(in);
      TRACE_LOG("[sound]repeats to play from file: %d.\n",
          nof_repeats_from_file);
      base_note = (int)fsi->getchar(in);
      frequency = ((int)fsi->getchar(in) << 8) | fsi->getchar(in);
      fsi->setfilepos(in, 2, SEEK_CUR);
      effect->data_len = ((int)fsi->getchar(in) << 8) | fsi->getchar(in);

      effect->is_internal_effect = false;
      effect->format = AUDIO_U8;
      effect->nof_channels = 1;
      effect->freq = frequency;
      effect->data = fizmo_malloc(effect->data_len);

      //delay = 0;
      fsi->getchars(effect->data, effect->data_len, in);
      fsi->closefile(in);

      if (repeats >= 1)
        effect->nof_repeats = (repeats == 255 ? -1 : repeats);
      else if (nof_repeats_from_file == 0)
        effect->nof_repeats = -1;
      else
        effect->nof_repeats = nof_repeats_from_file;

      TRACE_LOG("[sound]Repeats: %d, Volume: %d\n",
          effect->nof_repeats, effect->sound_volume);
    }
    else
    {
      TRACE_LOG("[sound]Not found.\n");
      free(effect);
      return;
    }
  }

  effect->sound_volume
    = round((float)SDL_MIX_MAXVOLUME / (float)8 * (float)volume);
  TRACE_LOG("[sound]volume:%d (%d).\n", effect->sound_volume, volume);
  effect->routine_after_playback = routine;

  SDL_mutexP(sound_output_active_mutex);

  sound_effect_top_element++;

/*
#ifdef ENABLE_TRACING

  TRACE_LOG("[sound]New effect index: %d.\n", sound_effect_top_element);

  time ( &rawtime );
  gettimeofday(&tv, &tz);
  tm=localtime(&tv.tv_sec);

  TRACE_LOG(
      "\n\n[sound](%s,%ld)store new effect (%d:%02d:%02d %d)\n\n",
      ctime (&rawtime), rawtime, tm->tm_hour, tm->tm_min,
      tm->tm_sec, tv.tv_usec);
#endif
  */

  if (sound_effect_top_element == sound_effect_fifo_size)
  {
    sound_effect_fifo_size += 4;

    TRACE_LOG("[sound]Resizing stack to %lu bytes.\n",
        sizeof(struct sound_effect*) * sound_effect_fifo_size);

    sdl_sound_effect_fifo = fizmo_realloc(
        sdl_sound_effect_fifo,
        sizeof(struct sound_effect*) * sound_effect_fifo_size);
  }
  TRACE_LOG("[sound]%d\n", sound_effect_top_element);
  sdl_sound_effect_fifo[sound_effect_top_element] = effect;

  if (bool_equal(sound_output_active, false))
  {
    if (effect_thread == NULL)
    {
      TRACE_LOG("[sound]Creating new effect_thread.\n");
      effect_thread = SDL_CreateThread(&effect_thread_routine, NULL);
    }
    start_next_effect();
    SDL_mutexV(sound_output_active_mutex);
  }
  else
  {
    if (read_has_occurred == false)
    {
      TRACE_LOG("[sound]New sound during same read cycle, starting flush.\n");
      flush_sound_effect_stack = true;
    }
    else
    {
      SDL_PauseAudio(1);
      clear_current_effect_from_stack();
      sound_output_active = false;
    }
    SDL_mutexV(sound_output_active_mutex);
  }
}


void sdl_stop_sound(int UNUSED(sound_nr))
{
  SDL_TimerID timer_to_terminate;

  if (sound_init_performed == false)
    return;

  SDL_mutexP(sound_output_active_mutex);
  timer_to_terminate = sdl_finish_timer;
  if (timer_to_terminate != NULL)
  {
    SDL_RemoveTimer(timer_to_terminate);
  }

  if (sound_output_active == true)
  {
    SDL_PauseAudio(1);
    clear_all_effects_from_stack();

    if (is_audio_open == true)
    {
      TRACE_LOG("[sound]errno: %d\n", errno);
      // FIXME: errno is set to 60 (Operation timed out) here:
      SDL_CloseAudio();
      TRACE_LOG("[sound]errno: %d\n", errno);
      errno = 0;
      is_audio_open = false;
    }

    sound_output_active = false;
  }
  SDL_mutexV(sound_output_active_mutex);
}


void sdl_finish_sound(int UNUSED(sound_nr))
{
  /*
  if (sound_init_performed == false)
    return;
  */
}


void sdl_keyboard_input_has_occurred()
{
  SDL_mutexP(sound_output_active_mutex);
  read_has_occurred = true;
  SDL_mutexV(sound_output_active_mutex);
}


uint16_t sdl_get_next_sound_end_routine()
{
  uint16_t result;

  SDL_mutexP(sound_output_active_mutex);

  if (routine_stack_top_element >= 0)
  {
    result = routine_stack[routine_stack_top_element];
    routine_stack_top_element--;
  }
  else
    result = 0;

  SDL_mutexV(sound_output_active_mutex);

  return result;
}


static char *get_sdl_interface_name()
{
  return sdl_interface_name;
}


static char *get_sdl_interface_version()
{
  return sdl_interface_version;
}


static int sdl_parse_config_parameter(char *key, char *value)
{
  if (strcasecmp(key, "force-8bit-sound") == 0)
  {
    if (
        (value == NULL)
        ||
        (*value == 0)
        ||
        (strcmp(value, config_true_value) == 0)
       )
      force_8bit_sound = true;
    else
      force_8bit_sound = false;
    free(value);
    return 0;
  }
  else
  {
    return -2;
  }
}


static char *sdl_get_config_value(char *key)
{
  if (strcasecmp(key, "force-8bit-sound") == 0)
  {
    return force_8bit_sound == true
      ? config_true_value
      : config_false_value;
  }
  else
  {
    return NULL;
  }
}


static char **sdl_get_config_option_names()
{
  return config_option_names;
}


struct z_sound_interface sound_interface_sdl =
{
  &sdl_init_sound,
  &sdl_close_sound,
  &sdl_prepare_sound,
  &sdl_play_sound,
  &sdl_stop_sound,
  &sdl_finish_sound,
  &sdl_keyboard_input_has_occurred,
  &sdl_get_next_sound_end_routine,
  &get_sdl_interface_name,
  &get_sdl_interface_version,
  &sdl_parse_config_parameter,
  &sdl_get_config_value,
  &sdl_get_config_option_names
};

//sound_interface_sdl = &sound_interface_sdl_struct;
//struct z_sound_interface *sound_interface_sdl = &sound_interface_sdl_struct;

#endif /* sound_sdl_c_INCLUDED */

