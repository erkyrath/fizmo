
/* glk_screen_if.h
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2011-2012 Andrew Plotkin and Christoph Ender.
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

#ifndef glk_screen_if_h_INCLUDED
#define glk_screen_if_h_INCLUDED

#include <screen_interface/screen_interface.h>

#ifndef glk_screen_if_c_INCLUDED
extern struct z_screen_interface glkint_screen_interface;
#endif // glk_screen_if_c_INCLUDED

typedef struct library_state_data_struct {
  int active; /* does this structure contain meaningful data? */
  int statusseenheight;
  int statusmaxheight;
  int statuscurheight;
} library_state_data;

z_file *glkint_open_interface(z_file *(*game_open_func)(z_file *));
void glkint_recover_library_state(library_state_data *dat);
void glkint_stash_library_state(library_state_data *dat);

#endif // glk_screen_if_h_INCLUDED

