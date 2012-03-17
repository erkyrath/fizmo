
/* tools_test.c
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


#ifndef tools_test_c_INCLUDED
#define tools_test_c_INCLUDED

#include <stdlib.h>

#include "i18n.h"
#include "tracelog.h"
#include "z_ucs.h"
#include "unused.h"
#include "../locales/libfizmo_locales.h"

#define STREAM_BUFSIZE 128


int i18n_test_stream_output(z_ucs *output)
{
  int len;
  char buf[STREAM_BUFSIZE];

  while (*output != 0)
  {
    len = zucs_string_to_utf8_string(buf, &output, STREAM_BUFSIZE);
    fwrite(buf, 1, len-1, stdout);
  }

  return 0;
}


int main(int UNUSED(argc), char *UNUSED(argv[]))
{
  z_ucs *path;
  z_ucs german_locale_name[] = { 'd', 'e', '_', 'D', 'E', 0 };

  printf("\nStarting tools-test.\n");

  turn_on_trace();

  if ((path = dup_latin1_string_to_zucs_string("../locales")) == NULL)
    return -1;
  set_i18n_search_path(path);
  free(path);

  register_i18n_stream_output_function(&i18n_test_stream_output);

  printf("\nTrying to display first localized error message:\n ");

  TRACE_LOG("Starting tools_test.\n");

  i18n_translate(
      libfizmo_module_name,
      i18n_libfizmo_COULD_NOT_OPEN_TRACE_FILE_P0S,
      "foo1");

  printf("\n\nTrying to display second localized error message:\n ");

  i18n_translate(
      libfizmo_module_name,
      i18n_libfizmo_INVALID_PARAMETER_TYPE_P0S,
      "foo2");

  if (set_current_locale_name(german_locale_name) != 0)
  {
    printf("set_current_locale_name() failed.\n");
    exit(-1);
  }

  printf("\n\nTrying to display german localized error message:\n ");

  i18n_translate(
      libfizmo_module_name,
      i18n_libfizmo_COULD_NOT_OPEN_TRACE_FILE_P0S,
      "foo1");

  printf("\n");

  TRACE_LOG("End of tools_test.\n");

  printf(
      "\nTools-test finished, you might want to examine \"tracelog.txt\".\n\n");

  return 0;
}


#endif /* tools_test_c_INCLUDED */

