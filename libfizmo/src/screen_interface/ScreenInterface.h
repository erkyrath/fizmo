
/* ScreenInterface.h
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


#ifndef ScreenInterface_h_INCLUDED 
#define ScreenInterface_h_INCLUDED

#include "types.h"


class ScreenInterface {
  public:
    virtual ~ScreenInterface();
    virtual char *get_interface_name();
    virtual bool is_status_line_available();
    virtual bool is_split_screen_available();
    virtual bool is_variable_pitch_font_default();
    virtual bool is_color_available();
    virtual bool is_picture_displaying_available();
    virtual bool is_bold_face_available();
    virtual bool is_italic_available();
    virtual bool is_fixed_space_font_available();
    virtual bool is_timed_keyboard_available();
    virtual bool is_preloaded_input_available();
    virtual bool is_character_graphics_font_availiable();
    virtual bool is_picture_font_availiable();
    virtual uint8_t get_screen_height();
    virtual uint8_t get_screen_width();
    virtual uint8_t get_screen_width_in_units();
    virtual uint8_t get_screen_height_in_units();
    virtual uint8_t get_font_width_in_units();
    virtual uint8_t get_font_height_in_units();
    virtual z_colour get_default_foreground_colour();
    virtual z_colour get_default_background_colour();
    virtual uint8_t get_total_width_in_pixels_of_text_sent_to_output_stream_3();
    virtual int parse_config_parameter(char *key, char *value);
    virtual char *get_config_value(char *key);
    virtual char **get_config_option_names();
    virtual void link_interface_to_story(struct z_story *story);
    virtual void reset_interface();
    virtual int close_interface(z_ucs *error_message);
    virtual void set_buffer_mode(uint8_t new_buffer_mode);
    virtual void z_ucs_output(z_ucs *z_ucs_output);
    virtual int16_t read_line(zscii *dest, uint16_t maximum_length,
        uint16_t tenth_seconds, uint32_t verification_routine,
        uint8_t preloaded_input, int *tenth_seconds_elapsed,
        bool disable_command_history, bool return_on_escape);
    virtual int read_char(uint16_t tenth_seconds,
        uint32_t verification_routine, int *tenth_seconds_elapsed);
    virtual void show_status(z_ucs *room_description, int status_line_mode,
        int16_t parameter1, int16_t parameter2);
    virtual void set_text_style(z_style text_style);
    virtual void set_colour(z_colour foreground, z_colour background,
        int16_t window);
    virtual void set_font(z_font font_type);
    virtual void split_window(int16_t nof_lines);
    virtual void set_window(int16_t window_number);
    virtual void erase_window(int16_t window_number);
    virtual void set_cursor(int16_t line, int16_t column, int16_t window);
    virtual uint16_t get_cursor_row();
    virtual uint16_t get_cursor_column();
    virtual void erase_line_value(uint16_t start_position);
    virtual void erase_line_pixels(uint16_t start_position);
    virtual void output_interface_info();
    virtual bool input_must_be_repeated_by_story();
};

#endif /* ScreenInterface_h_INCLUDED */

