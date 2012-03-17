
/* fizmo_console.c
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


#ifdef ENABLE_READCHAR_VIA_TERMIOS
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#endif // ENABLE_READCHAR_VIA_TERMIOS

#include <string.h>

#include <interpreter/fizmo.h>
#include <interpreter/text.h>
#include <interpreter/streams.h>
#include <interpreter/wordwrap.h>
#include <tools/unused.h>
#include <tools/types.h>
#include <tools/i18n.h>
#include <tools/tracelog.h>

static char* interface_name = "fizmo-console";
static char* interface_version = "0.7.1";
int line_length = -1;
bool disable_hyphenation = false;
static WORDWRAP *output_wordwrapper;


static char *simple_c_get_interface_name()
{ return interface_name; }

static bool simple_c_return_false()
{ return false; }

static bool simple_c_return_true()
{ return true; }

static uint8_t simple_c_get_screen_height()
{ return 24; }

static uint8_t simple_c_get_screen_width()
{ return 80; }

static uint8_t simple_c_return_1()
{ return 1; }

static z_colour simple_c_get_default_foreground_colour()
{ return Z_COLOUR_BLACK; }

static z_colour simple_c_get_default_background_colour()
{ return Z_COLOUR_WHITE; }

static uint8_t simple_c_return_0()
{ return 0; }

static int simple_c_parse_config_parameter(char *UNUSED(key),
    char *UNUSED(value))
{ return 1; }

static char *simple_c_get_config_value(char *UNUSED(key))
{ return NULL; }

static char **simple_c_get_config_option_names()
{ return NULL; }

static void simple_c_link_interface_to_story(struct z_story *UNUSED(story))
{
  putc('\n', stdout);
}

static void simple_c_reset_interface()
{ }

static void simple_c_set_buffer_mode(uint8_t UNUSED(new_buffer_mode))
{ }

static void z_ucs_output_wordwrap_destination(z_ucs *z_ucs_output,
    void *UNUSED(dummyparameter))
{
  while (*z_ucs_output != 0)
  {
    if ((*z_ucs_output & 0xffffff80) != 0)
      putchar('?');
    else
      putchar((char)*z_ucs_output);

    z_ucs_output++;
  }
}

static void simple_c_interface_output_z_ucs(z_ucs *z_ucs_output)
{
  if (line_length == -1)
    z_ucs_output_wordwrap_destination(z_ucs_output, NULL);
  else
    wordwrap_wrap_z_ucs(output_wordwrapper, z_ucs_output);
}

static int simple_c_close_interface(z_ucs *error_message)
{
  if (error_message != NULL)
    simple_c_interface_output_z_ucs(error_message);
  return 0;
}

static int16_t simple_c_interface_read_line(zscii *dest,
    uint16_t maximum_length, uint16_t UNUSED(tenth_seconds),
    uint32_t UNUSED(verification_routine), uint8_t UNUSED(preloaded_input),
    int *UNUSED(tenth_seconds_elapsed), bool UNUSED(disable_command_history),
    bool UNUSED(return_on_escape))
{
  int input;
  int current_length = 0;
  uint8_t input_size = 0;
  zscii input_zscii;

  if (line_length != -1)
    wordwrap_flush_output(output_wordwrapper);

  if ((input = fgetc(stdin)) == EOF)
    exit(-1);

  while ((input != 10) && (input != 13))
  {
    if (current_length < maximum_length)
    {
      input_zscii = unicode_char_to_zscii_input_char(input & 0xff);

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

static int simple_c_interface_read_char(uint16_t UNUSED(tenth_seconds),
    uint32_t UNUSED(verification_routine), int *UNUSED(tenth_seconds_elapsed))
{
  int result;
#ifdef ENABLE_READCHAR_VIA_TERMIOS
  struct termios org_opts, new_opts;
#endif // ENABLE_READCHAR_VIA_TERMIOS

  if (line_length != -1)
    wordwrap_flush_output(output_wordwrapper);

#ifdef ENABLE_READCHAR_VIA_TERMIOS

  tcgetattr(STDIN_FILENO, &org_opts);
  memcpy(&new_opts, &org_opts, sizeof(new_opts));
  new_opts.c_lflag
    &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
  result = getchar();
  result = unicode_char_to_zscii_input_char(result & 0xff);
  if (result == 0xff)
    result = -1;
  tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);

  return result;

#else

  zscii input_zscii;
  int input;

  if ((input = fgetc(stdin)) == EOF)
    exit(-1);

  result = -1;

  while ((input != 10) && (input != 13))
  {
    if (result == -1)
    {
      if ((input_zscii=unicode_char_to_zscii_input_char(input & 0xff)) != 0xff)
        result = input_zscii;
    }

    if ((input = fgetc(stdin)) == EOF)
      exit(-1);
  }

  if (result == -1)
    result = 10;

#endif // ENABLE_READCHAR_VIA_TERMIOS

  return result;
}

static void simple_c_show_status(z_ucs *UNUSED(room_description),
    int UNUSED(status_line_mode), int16_t UNUSED(parameter1),
    int16_t UNUSED(parameter2))
{ }

static void simple_c_set_text_style(z_style UNUSED(text_style))
{ }

static void simple_c_set_colour(z_colour UNUSED(foreground),
    z_colour UNUSED(background), int16_t UNUSED(window))
{ }

static void simple_c_set_font(z_font UNUSED(font_type))
{ }

static void simple_c_split_window(int16_t UNUSED(nof_lines))
{ }

static void simple_c_set_window(int16_t UNUSED(window_number))
{ }

static void simple_c_erase_window(int16_t UNUSED(window_number))
{ }

static void simple_c_set_cursor(int16_t UNUSED(line), int16_t UNUSED(column),
    int16_t UNUSED(window))
{ }

static uint16_t simple_c_get_cursor_row()
{ return 0; }

static uint16_t simple_c_get_cursor_column()
{ return 0;}

static void simple_c_erase_line_value(uint16_t UNUSED(start_position))
{ }

static void simple_c_erase_line_pixels(uint16_t UNUSED(start_position))
{ }

static void simple_c_output_interface_info()
{
  (void)streams_latin1_output(interface_name);
  (void)streams_latin1_output(" interface version ");
  (void)streams_latin1_output(interface_version);
  (void)streams_latin1_output("\n");
}

static void simple_c_game_was_restored_and_history_modified()
{ }

struct z_screen_interface simple_c_interface =
{
  &simple_c_get_interface_name,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_true,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_return_false,
  &simple_c_get_screen_height,
  &simple_c_get_screen_width,
  &simple_c_get_screen_height,
  &simple_c_get_screen_width,
  &simple_c_return_1,
  &simple_c_return_1,
  &simple_c_get_default_foreground_colour,
  &simple_c_get_default_background_colour,
  &simple_c_return_0,
  &simple_c_parse_config_parameter,
  &simple_c_get_config_value,
  &simple_c_get_config_option_names,
  &simple_c_link_interface_to_story,
  &simple_c_reset_interface,
  &simple_c_close_interface,
  &simple_c_set_buffer_mode,
  &simple_c_interface_output_z_ucs,
  &simple_c_interface_read_line,
  &simple_c_interface_read_char,
  &simple_c_show_status,
  &simple_c_set_text_style,
  &simple_c_set_colour,
  &simple_c_set_font,
  &simple_c_split_window,
  &simple_c_set_window,
  &simple_c_erase_window,
  &simple_c_set_cursor,
  &simple_c_get_cursor_row,
  &simple_c_get_cursor_column,
  &simple_c_erase_line_value,
  &simple_c_erase_line_pixels,
  &simple_c_output_interface_info,
  &simple_c_return_false,
  &simple_c_game_was_restored_and_history_modified
};


static void print_syntax()
{
  puts("\nUsage: fizmo-console [options] <story-file>\n");

  printf("fizmo-console version %s.\n", interface_version);
  printf("libfizmo version %s.\n\n", FIZMO_VERSION);

  puts("Valid options are:");
  puts(" -ll, --line-length: Wrap words into lines of given length.");
  puts(" -dh, --disable-hyphenation: Disable hyphenation.");
  puts(" -h,  --help: Show help message and exit.\n");
}


int main(int argc, char *argv[])
{
  z_file *story_stream;
  char *story_filename = NULL;
  int i;

#ifdef ENABLE_TRACING
  turn_on_trace();
#endif // ENABLE_TRACING

  fizmo_register_screen_interface(&simple_c_interface);

  for (i=1; i<argc; i++)
  {
    if ( (strcmp(argv[i], "-h") == 0)
        || (strcmp(argv[i], "--help") == 0) )
    {
      print_syntax();
      exit(EXIT_SUCCESS);
    }
    else if ( (strcmp(argv[i], "-ll") == 0)
        || (strcmp(argv[i], "--line-length") == 0) )
    {
      if (++i == argc)
      {
        print_syntax();
        exit(EXIT_FAILURE);
      }

      line_length = atoi(argv[i]);
    }
    else if ( (strcmp(argv[i], "-dh") == 0)
        || (strcmp(argv[i], "--disable-hyphenation") == 0) )
    {
      disable_hyphenation = true;
    }
    else
    {
      if (story_filename != NULL)
      {
        print_syntax();
        exit(EXIT_FAILURE);
      }
      story_filename = argv[i];
    }
  }

  if (story_filename == NULL)
  {
    print_syntax();
    exit(EXIT_FAILURE);
  }

  if (line_length > 0)
  {
    output_wordwrapper = wordwrap_new_wrapper(
        line_length,
        &z_ucs_output_wordwrap_destination,
        (void*)NULL,
#if defined (__WIN32__)
        false,
#else
        true,
#endif // defined (__WIN32__)
        0,
        false,
        disable_hyphenation == false ? true : false);
  }

  if ((story_stream
      = fsi->openfile(story_filename, FILETYPE_DATA, FILEACCESS_READ)) == NULL)
  {
    printf("Could not open file \"%s\".\n", argv[1]);
    return -1;
  }
  else
  {
    fizmo_start(story_stream, NULL, NULL, -1, -1);
    return 0;
  }
}

