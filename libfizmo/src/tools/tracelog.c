
/* tracelog.c
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
 *
 *
 * Why tracelog at all? Some errors, for example when resizing screens or
 * other procedures, simply do not occur when running under gdb, so an
 * extra debug method is very welcome. Furthermore, the tracelog also
 * allows to get a very detailed log of a user's error without having to
 * install or explain a debugger fist.
 *
 */


#ifndef tracelog_c_INCLUDED
#define tracelog_c_INCLUDED

#include <string.h>
#include <stdio.h>

#include "tracelog.h"
#include "i18n.h"
#include "types.h"
#include "z_ucs.h"
#include "../locales/libfizmo_locales.h"


FILE *stream_t = NULL;

#ifdef ENABLE_TRACING

void turn_on_trace(void)
{
  if (stream_t != NULL)
  {
    TRACE_LOG("Tracelog already active.\n");
    return;
  }

  stream_t = fopen(DEFAULT_TRACE_FILE_NAME, "w");

  if (stream_t == NULL)
    i18n_translate_and_exit(
        libfizmo_module_name,
        i18n_libfizmo_COULD_NOT_OPEN_TRACE_FILE_P0S,
        -1,
        DEFAULT_TRACE_FILE_NAME);
}


void turn_off_trace(void)
{
  if (stream_t == NULL)
  {
    TRACE_LOG("Tracelog already deactivated.\n");
    return;
  }

  fflush(stream_t);
  fclose(stream_t);

  stream_t = NULL;
}


void _trace_log_z_ucs(z_ucs *output)
{
  char utf_8_output[80 * UTF_8_MB_LEN + 1];

  if (stream_t != NULL)
  {
    while (*output != 0)
    {
      zucs_string_to_utf8_string(utf_8_output, &output, 80 * UTF_8_MB_LEN + 1);
      fputs(utf_8_output, stream_t);
      fflush(stream_t);
    }
  }
}

#endif /* ENABLE_TRACING */

#endif // tracelog_c_INCLUDED

