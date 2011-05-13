
/* config.h
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


#ifndef config_h_INCLUDED 
#define config_h_INCLUDED

#include "../tools/types.h"

#define FIZMO_CONFIG_DIR_NAME "fizmo"
#define DEFAULT_XDG_CONFIG_HOME ".config"
#define SYSTEM_FIZMO_CONF_FILE_NAME "/etc/fizmo.conf"
#define DEFAULT_XDG_CONFIG_DIRS "/etc/xdg"
#define FIZMO_CONFIG_FILE_NAME "config"
#define BABEL_TIMESTAMP_FILE_NAME "babel-timestamps"

#define NUMBER_OF_FILES_TO_SHOW_PROGRESS_FOR 10

#define DEFAULT_LOCALE "en_US"
#define FIZMO_COMMAND_PREFIX '/'
#define Z_STACK_INCREMENT_SIZE 64

#define MAXIMUM_SAVEGAME_NAME_LENGTH 64
#define DEFAULT_SAVEGAME_FILENAME "savegame.qut"

#define MAXIMUM_SCRIPT_FILE_NAME_LENGTH 64
#define DEFAULT_SCRIPT_FILE_NAME "script.txt"
#define DEFAULT_COMMAND_FILE_NAME "record.txt"

#define DEFAULT_STREAM_2_LINE_WIDTH 78
// Note that some games (Aisle may be an example) format their output
// (the title screen) according to the current screen width.
#define DEFAULT_STREAM_2_LEFT_PADDING 1

#define DEFAULT_TRACE_FILE_NAME "tracelog.txt"

#define MAXIMUM_STREAM_3_DEPTH 16

#define NUMBER_OF_REMEMBERED_COMMANDS 100
#define COMMAND_HISTORY_SIZE_INCREASE 1024

#define SYSTEM_CHARSET_ASCII 0
#define SYSTEM_CHARSET_ISO_8859_1 1
#define SYSTEM_CHARSET_UTF_8 2

// Assuming that the maximum dynamic memory of a story may be 100kb and
// that we'll spare 10 Megabytes of RAM for undo purposes we can store
// a maximum of 10240 undo steps.
#define MAX_UNDO_STEPS 10240

#define RANDOM_SEED_SIZE 32

// Ignorig this error makes "HugeCave.z8" work.
#define IGNORE_TOO_LONG_PROPERTIES_ERROR 1

//#define THROW_SIGFAULT_ON_ERROR 1

struct configuration_option
{
  char *name;
  char *value;
};

#ifndef config_c_INCLUDED 
extern int system_charset;
extern bool auto_adapt_upper_window;
extern bool auto_open_upper_window;
extern bool skip_active_routines_stack_check_warning;
extern char true_value[];
extern char false_value[];
extern struct configuration_option configuration_options[];
#endif /* config_c_INCLUDED */

int set_configuration_value(char *key, char* new_value);
int append_path_value(char *key, char *value_to_append);
char *get_configuration_value(char *key);
//char **get_valid_configuration_options(char *key, ...);

#endif /* config_h_INCLUDED */

