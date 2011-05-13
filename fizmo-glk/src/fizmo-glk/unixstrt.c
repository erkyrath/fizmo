
/* unixstrt.c
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2011 Andrew Plotkin.
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


#include "glk.h"
#include "glkint.h"
#include "glkstart.h" /* This comes with the Glk library. */

#include <interpreter/fizmo.h>
#include <tools/types.h>

static char *game_filename = NULL;
static char *init_err = NULL; /*### use this */
static char *init_err2 = NULL; /*### use this */

static void stream_hexnum(glsi32 val);

glkunix_argumentlist_t glkunix_arguments[] = {
    { "", glkunix_arg_ValueFollows, "filename: The game file to load." },
    { NULL, glkunix_arg_End, NULL }
};

int glkunix_startup_code(glkunix_startup_t *data)
{
    /* It turns out to be more convenient if we return TRUE from here, even 
       when an error occurs, and display an error in glk_main(). */
    int ix;
    char *filename = NULL;

    /* Parse out the arguments. They've already been checked for validity,
       and the library-specific ones stripped out.
       As usual for Unix, the zeroth argument is the executable name. */
    for (ix=1; ix<data->argc; ix++) {
        if (filename) {
            init_err = "You must supply exactly one game file.";
            return TRUE;
        }
        filename = data->argv[ix];
    }

    if (!filename) {
        init_err = "You must supply the name of a game file.";
        return TRUE;
    }

    game_filename = filename;
    
    return TRUE;
}

void glk_main(void)
{
#ifdef ENABLE_TRACING
    turn_on_trace();
#endif // ENABLE_TRACING

    if (init_err) {
        fatal_error_handler(init_err, init_err2, FALSE, 0);
        return;
    }

    fizmo_register_screen_interface(&glkint_interface);

    glkint_open_interface();
    fizmo_start(game_filename, NULL, NULL);
}

/* get_error_win():
   Return a window in which to display errors. The first time this is called,
   it creates a new window; after that it returns the window it first
   created.
*/
static winid_t get_error_win()
{
    static winid_t errorwin = NULL;

    if (!errorwin) {
        winid_t rootwin = glk_window_get_root();
        if (!rootwin) {
            errorwin = glk_window_open(0, 0, 0, wintype_TextBuffer, 1);
        }
        else {
            errorwin = glk_window_open(rootwin, winmethod_Below | winmethod_Fixed, 
                3, wintype_TextBuffer, 0);
        }
        if (!errorwin)
            errorwin = rootwin;
    }

    return errorwin;
}

/* fatal_error_handler():
   Display an error in the error window, and then exit.
*/
void fatal_error_handler(char *str, char *arg, int useval, glsi32 val)
{
    winid_t win = get_error_win();
    if (win) {
        glk_set_window(win);
        glk_put_string("Fizmo fatal error: ");
        glk_put_string(str);
        if (arg || useval) {
            glk_put_string(" (");
            if (arg)
                glk_put_string(arg);
            if (arg && useval)
                glk_put_string(" ");
            if (useval)
                stream_hexnum(val);
            glk_put_string(")");
        }
        glk_put_string("\n");
    }
    glk_exit();
}

/* stream_hexnum():
   Write a signed integer to the current Glk output stream.
*/
static void stream_hexnum(glsi32 val)
{
    char buf[16];
    glui32 ival;
    int ix;

    if (val == 0) {
        glk_put_char('0');
        return;
    }

    if (val < 0) {
        glk_put_char('-');
        ival = -val;
    }
    else {
        ival = val;
    }

    ix = 0;
    while (ival != 0) {
        buf[ix] = (ival % 16) + '0';
        if (buf[ix] > '9')
            buf[ix] += ('A' - ('9' + 1));
        ix++;
        ival /= 16;
    }

    while (ix) {
        ix--;
        glk_put_char(buf[ix]);
    }
}

