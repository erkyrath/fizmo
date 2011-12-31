
/* ScreenInterface.cpp
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


#include <iostream>
#include <stdlib.h>
#include "ScreenInterface.h"
#include "Interpreter.h"

ScreenInterface::~ScreenInterface()
{ }

char *ScreenInterface::get_interface_name()
{ return NULL; }

bool ScreenInterface::is_status_line_available()
{ return false; }

bool ScreenInterface::is_split_screen_available()
{ return false; }

bool ScreenInterface::is_variable_pitch_font_default()
{ return false; }

bool ScreenInterface::is_color_available()
{ return false; }

bool ScreenInterface::is_picture_displaying_available()
{ return false; }

bool ScreenInterface::is_bold_face_available()
{ return false; }

bool ScreenInterface::is_italic_available()
{ return false; }

bool ScreenInterface::is_fixed_space_font_available()
{ return false; }

bool ScreenInterface::is_timed_keyboard_available()
{ return false; }

bool ScreenInterface::is_preloaded_input_available()
{ return false; }

bool ScreenInterface::is_character_graphics_font_availiable()
{ return false; }

bool ScreenInterface::is_picture_font_availiable()
{ return false; }

uint8_t ScreenInterface::get_screen_height()
{ return 0; }

uint8_t ScreenInterface::get_screen_width()
{ return 0; }

uint8_t ScreenInterface::get_screen_width_in_units()
{ return 0; }

uint8_t ScreenInterface::get_screen_height_in_units()
{ return 0; }

uint8_t ScreenInterface::get_font_width_in_units()
{ return 0; }

uint8_t ScreenInterface::get_font_height_in_units()
{ return 0; }

z_colour ScreenInterface::get_default_foreground_colour()
{ return Z_COLOUR_BLACK; }

z_colour ScreenInterface::get_default_background_colour()
{ return Z_COLOUR_WHITE; }

uint8_t ScreenInterface::get_total_width_in_pixels_of_text_sent_to_output_stream_3()
{ return 0; }

int ScreenInterface::parse_config_parameter(char *key, char *value)
{ return 1; }

char *ScreenInterface::get_config_value(char *key)
{ return NULL; }

char **ScreenInterface::get_config_option_names()
{ return NULL; }

void ScreenInterface::link_interface_to_story(struct z_story *story)
{ }

void ScreenInterface::reset_interface()
{ }

int ScreenInterface::close_interface(z_ucs *error_message)
{ return 0; }

void ScreenInterface::set_buffer_mode(uint8_t new_buffer_mode)
{ }

void ScreenInterface::z_ucs_output(z_ucs *z_ucs_output)
{
  while (*z_ucs_output != 0)
  {
    if ((*z_ucs_output & 0xffffff80) != 0)
      fputc('?', stdout);
    else
      fputc(*z_ucs_output & 0x7f, stdout);

    z_ucs_output++;
  }
}

int16_t ScreenInterface::read_line(zscii *dest, uint16_t maximum_length,
    uint16_t tenth_seconds, uint32_t verification_routine,
    uint8_t preloaded_input, int *tenth_seconds_elapsed,
    bool disable_command_history, bool return_on_escape)
{
  int input;
  int current_length = 0;
  uint8_t input_size = 0;
  zscii input_zscii;

  if ((input = fgetc(stdin)) == EOF)
  {
    exit(-1);
  }

  while ((input != 10) && (input != 13))
  {
    if (current_length < maximum_length)
    {
      input_zscii = Interpreter::unicode_char_to_zscii_input_char(input & 0xff);

      if ((input_zscii == 0xff) || (input_zscii == 27))
        input_zscii = '?';

      *(dest++) = input_zscii;

      input_size++;
    }

    if ((input = fgetc(stdin)) == EOF)
      exit(-1);
  }

  return input_size;
}

int ScreenInterface::read_char(uint16_t tenth_seconds,
    uint32_t verification_routine, int *tenth_seconds_elapsed)
{ return 0; }

void ScreenInterface::show_status(z_ucs *room_description, int status_line_mode,
    int16_t parameter1, int16_t parameter2)
{ }

void ScreenInterface::set_text_style(z_style text_style)
{ }

void ScreenInterface::set_colour(z_colour foreground, z_colour background,
    int16_t window)
{ }

void ScreenInterface::set_font(z_font font_type)
{ }

void ScreenInterface::split_window(int16_t nof_lines)
{ }

void ScreenInterface::set_window(int16_t window_number)
{ }

void ScreenInterface::erase_window(int16_t window_number)
{ }

void ScreenInterface::set_cursor(int16_t line, int16_t column, int16_t window)
{ }

uint16_t ScreenInterface::get_cursor_row()
{ return 0; }

uint16_t ScreenInterface::get_cursor_column()
{ return 0; }

void ScreenInterface::erase_line_value(uint16_t start_position)
{ }

void ScreenInterface::erase_line_pixels(uint16_t start_position)
{ }

void ScreenInterface::output_interface_info()
{ }

bool ScreenInterface::input_must_be_repeated_by_story()
{ return false; }

