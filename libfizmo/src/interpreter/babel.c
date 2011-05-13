
/* babel.c
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


#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#ifndef DISABLE_LIBXML2
#include <dirent.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#endif // DISABLE_LIBXML2

#include "babel.h"
#include "config.h"
#include "fizmo.h"
#include "../tools/tracelog.h"

static FILE *timestamp_file;
static struct babel_timestamp_entry *babel_timestamp_entries;
static char *timestamp_input = NULL;
static int timestamp_input_size = 0;
static char *filename_input = NULL;
static int filename_input_size = 0;
static char *unquoted_filename_input = NULL;




void free_babel_story_info(struct babel_story_info *b_info)
{
  free(b_info->serial);
  free(b_info->title);
  free(b_info->author);
  free(b_info->description);
  free(b_info);
}


void free_babel_doc_entry(struct babel_doc_entry *entry)
{
  if (entry == NULL)
    return;

#ifndef DISABLE_LIBXML2
  xmlFreeDoc(entry->babel_doc);
#endif
  free(entry->filename);
  free(entry);
}


void free_babel_info(struct babel_info *babel)
{
  int i;

  if (babel == NULL)
    return;

  for (i=0; i<babel->nof_entries; i++)
    free_babel_doc_entry(babel->entries[i]);
  free(babel->entries);
  free(babel);
}


/*
struct babel_info *load_babel_info_from_blorb(int fd)
{
  struct babel_info *result;
  //char *xmlData = (char*)fizmo_malloc(length);
  xmlDocPtr babel_doc;

  result = (struct babel_info*)fizmo_malloc(sizeof(struct babel_info));

  if ((babel_doc = xmlReadFd(
          //fd, NULL, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR))
      fd, NULL, NULL, 0))
        != NULL)
  {
  }
  else
  {
    printf("xmlerror\n");
  }

  return result;
}
*/


#ifndef DISABLE_LIBXML2
static int add_doc_to_babel_info(xmlDocPtr new_babel_doc,
    struct babel_info *babel, struct stat *stat_buf, char *filename)
{
  xmlXPathContextPtr xpathCtx; 
  xmlXPathObjectPtr xpathObj;
  char *xmlExpr = "/ifindex";
  char *xmlNamespacedExpr = "/if:ifindex";
  bool uses_if_namespace;
  struct babel_doc_entry *new_entry;

  // Check contents
  xpathCtx = xmlXPathNewContext(new_babel_doc);

  if(xpathCtx == NULL)
  {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    return -1;
  }

  if (xmlXPathRegisterNs(
        xpathCtx,
        (xmlChar*)"if",
        (xmlChar*)"http://babel.ifarchive.org/protocol/iFiction/") == -1)
  {
    fprintf(stderr,"Error: unable to create new namespace\n");
    xmlXPathFreeContext(xpathCtx); 
    return -1;
  }

  xpathObj = xmlXPathEvalExpression((xmlChar*)xmlExpr, xpathCtx);
  if (xpathObj == NULL)
  {
    xmlXPathFreeContext(xpathCtx); 
    fprintf(
        stderr,
        "Error: unable to evaluate xpath expression \"%s\"\n",
        xmlExpr);
    return -1;
  }

  if (xmlXPathNodeSetGetLength(xpathObj->nodesetval) != 1)
  {
    // "/ifindex" was not found. Try "/if:ifindex".
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression(
        (xmlChar*)xmlNamespacedExpr, xpathCtx);

    if (xpathObj == NULL)
    {
      xmlXPathFreeContext(xpathCtx); 
      fprintf(
          stderr,
          "Error: unable to evaluate xpath expression \"%s\"\n",
          xmlNamespacedExpr);
      return -1;
    }

    if (xmlXPathNodeSetGetLength(xpathObj->nodesetval) != 1)
    {
      // Neither "/ifindex" nor "/if:ifindex" found. Skip this file.
      xmlXPathFreeObject(xpathObj);
      xmlXPathFreeContext(xpathCtx); 
      xmlFreeDoc(new_babel_doc);
      return -1;
    }
    else
      uses_if_namespace = true;
  }
  else
    uses_if_namespace = false;

  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx); 

  if (babel->nof_entries == babel->entries_allocated)
  {
    babel->entries = (struct babel_doc_entry**)fizmo_realloc(
        babel->entries,
        sizeof(struct babel_doc_entry*) * (babel->entries_allocated+10));
    babel->entries_allocated += 10;
  }

  new_entry = (struct babel_doc_entry*)fizmo_malloc(
      sizeof(struct babel_doc_entry));

  new_entry->babel_doc = new_babel_doc;
  new_entry->uses_if_namespace = uses_if_namespace;
  new_entry->timestamp = stat_buf->st_ctime;
  new_entry->filename = fizmo_strdup(filename);

  babel->entries[babel->nof_entries] = new_entry;
  babel->nof_entries++;

  return 0;
}
#endif


#ifndef DISABLE_LIBXML2
struct babel_info *load_babel_info_from_blorb(FILE *infile, int length,
    char *filename, struct stat *stat_buf)
{
  struct babel_info *result;
  //struct babel_doc_entry *new_entry;
  char *xmlData = (char*)fizmo_malloc(length + 1);
  xmlDocPtr babel_doc;
  //struct stat stat_buf;

  if (fread(xmlData, length, 1, infile) != 1)
  {
    free(xmlData);
    return NULL;
  }

  /*
  if ((fstat(fileno(infile), &stat_buf)) != 0)
  {
    free(xmlData);
    return NULL;
  }
  */

  xmlData[length] = '\0';

  if ((babel_doc = xmlReadDoc(
          (xmlChar*)xmlData,
          NULL,
          NULL,
          XML_PARSE_NOWARNING | XML_PARSE_NOERROR))
      == NULL)
  {
    free(xmlData);
    return NULL;
  }
  //printf("(%s)\n", xmlData);

  result = (struct babel_info*)fizmo_malloc(sizeof(struct babel_info));
  result->entries = NULL;
  result->entries_allocated = 0;
  result->nof_entries = 0;

  if (add_doc_to_babel_info(babel_doc, result, stat_buf, filename) != 0)
  {
    free(result);
    free(xmlData);
    return NULL;
  }

  return result;
}
#else
struct babel_info *load_babel_info_from_blorb(FILE *UNUSED(infile),
    int UNUSED(length), char *UNUSED(filename), struct stat *UNUSED(stat_buf))
{
  return NULL;
}
#endif


struct babel_info *load_babel_info()
{
  struct babel_info *result = NULL;

#ifndef DISABLE_LIBXML2
  char *cwd = NULL;
  char *config_dir_name = get_fizmo_config_dir_name();
  DIR *config_dir;
  struct dirent dir_entry;
  struct dirent* ptr;
  struct stat stat_buf;
  int fildes;
  int return_code;
  xmlDocPtr new_babel_doc;

  if ((config_dir = opendir(config_dir_name)) == NULL)
    return NULL;

  cwd = getcwd(NULL, 0);
  if (chdir(config_dir_name) != 0)
  {
    closedir(config_dir);
    free(cwd);
    return NULL;
  }

  result = (struct babel_info*)fizmo_malloc(sizeof(struct babel_info));

  result->entries = NULL;
  result->entries_allocated = 0;
  result->nof_entries = 0;

  while (
      (readdir_r(config_dir, &dir_entry, &ptr) == 0)
      &&
      (ptr != NULL)
      )
  {
    if ((fildes = open(dir_entry.d_name, O_RDONLY)) < 0)
      continue;
    return_code = fstat(fildes, &stat_buf);
    close(fildes);

    if (
        (return_code == 0)
        &&
        ((stat_buf.st_mode & S_IFDIR) == 0)
        &&
        (strlen(dir_entry.d_name) >= 9)
        &&
        (strcasecmp(
                    dir_entry.d_name + strlen(dir_entry.d_name) - 9,
                    ".iFiction") == 0)
       )
    {
      if ((new_babel_doc = xmlReadFile(
              dir_entry.d_name, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR))
          != NULL)
      {
        if ((add_doc_to_babel_info(
                new_babel_doc, result, &stat_buf, dir_entry.d_name)) != 0)
        {
          free_babel_info(result);
          chdir(cwd);
          free(cwd);
          closedir(config_dir);
          return NULL;
        }
      }
    }
  }

  chdir(cwd);
  free(cwd);
  closedir(config_dir);

#endif

  return result;
}


#ifndef DISABLE_LIBXML2
static char *getStoryNodeContent(xmlXPathContextPtr xpathCtx, char *nodeName,
    char *namespace)
{
  xmlXPathObjectPtr obj2;
  xmlNodePtr node;
  char *ptr;
  char expr[160]; // FIXME: Calculate length.
  int i;
  char *result = NULL;
  int result_size = 0;
  int index = 0;
  bool last_was_space;
  bool is_first_char;
  int len;
  char *src;

  sprintf(
      expr,
      "%sbibliographic/%s%s/child::text()|%sbibliographic/%s%s/*",
      namespace,
      namespace,
      nodeName,
      namespace,
      namespace,
      nodeName);

  obj2 = xmlXPathEvalExpression(
      (xmlChar*)expr,
      xpathCtx);

  //printf("# %d.\n", xmlXPathNodeSetGetLength(obj2->nodesetval));
  for (i=0; i<xmlXPathNodeSetGetLength(obj2->nodesetval); i++)
  {
    node = obj2->nodesetval->nodeTab[i];

    if(node->type == XML_ELEMENT_NODE)
    {
      if (strcasecmp((char*)node->name, "br") == 0)
      {
        ensure_mem_size(&result, &result_size, index+1);
        result[index++] = '\n';
        //result[index++] = 0xa0;
      }
    }
    else if (node->type == XML_TEXT_NODE)
    {
      ptr = (char*)xmlNodeGetContent(node);
      len = strlen(ptr);
      src = ptr;
      ensure_mem_size(&result, &result_size, index + len + 1);
      last_was_space = false;
      is_first_char = true;
      while (*src != '\0')
      {
        if ( (*src == '\n') || (*src == '\r') || (*src == '\t')
            || (*src == ' ') )
        {
          if ( (last_was_space == false) && (is_first_char == false) )
          {
            last_was_space = true;
          }
        }
        else
        {
          if (last_was_space == true)
          {
            result[index] = ' ';
            index++;
          }
          last_was_space = false;

          result[index] = *src;
          is_first_char = false;
          index++;
        }
        src++;
      }
      result[index] = 0;

      xmlFree(ptr);
    }
  }

  //printf("(%s)\n", result);

  xmlXPathFreeObject(obj2);

  return result;
}
#endif // DISABLE_LIBXML2


#ifndef DISABLE_LIBXML2
struct babel_story_info *get_babel_story_info(uint16_t release, char *serial,
    uint16_t checksum, struct babel_info *babel, bool babel_from_blorb)
{
  xmlXPathContextPtr xpathCtx; 
  xmlXPathObjectPtr xpathObj;
  char *xmlNamespacedExpr="/if:ifindex/if:story[if:identification/if:ifid='ZCODE-%d-%6s' or translate(if:identification/if:ifid, 'ABCDEF', 'abcdef')=translate('ZCODE-%d-%6s-%04x', 'ABCDEF', 'abcdef')]";
  char *xmlExpr="/ifindex/story[identification/ifid='ZCODE-%d-%6s' or translate(identification/ifid, 'ABCDEF', 'abcdef')=translate('ZCODE-%d-%6s-%04x', 'ABCDEF', 'abcdef')]";
  char expr[strlen(xmlExpr) + 29];
  char namespacedExpr[strlen(xmlNamespacedExpr) + 29];
  char *current_expr;
  struct babel_story_info *result = NULL;
  int i;
  char if_namespace[] = "if:";
  char no_namespace[] = "";
  char *current_namespace;

  if (babel == NULL)
    return NULL;

  LIBXML_TEST_VERSION

  if (strlen(serial) > 6)
    return NULL;

  if (babel_from_blorb == false)
  {
    sprintf(expr, xmlExpr, release, serial, release, serial, checksum);
    sprintf(namespacedExpr, xmlNamespacedExpr, release, serial, release,
        serial, checksum);
  }
  else
  {
    sprintf(expr, "/ifindex/story");
    sprintf(namespacedExpr, "/if:ifindex/if:story");
  }

  //printf("(%s)\n", expr);
  //printf("(%s)\n", namespacedExpr);

  for (i=0; i<babel->nof_entries; i++)
  {
    xpathCtx = xmlXPathNewContext(babel->entries[i]->babel_doc);

    if(xpathCtx == NULL)
    {
      fprintf(stderr,"Error: unable to create new XPath context\n");
      return NULL;
    }

    if (babel->entries[i]->uses_if_namespace == true)
    {
      if (xmlXPathRegisterNs(
            xpathCtx,
            (xmlChar*)"if",
            (xmlChar*)"http://babel.ifarchive.org/protocol/iFiction/") == -1)
      {
        fprintf(stderr,"Error: unable to create new namespace\n");
        xmlXPathFreeContext(xpathCtx); 
        return NULL;
      }
      current_expr = namespacedExpr;
      current_namespace = if_namespace;
    }
    else
    {
      current_expr = expr;
      current_namespace = no_namespace;
    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)current_expr, xpathCtx);
    if (xpathObj == NULL)
    {
      xmlXPathFreeContext(xpathCtx); 
      fprintf(
          stderr,
          "Error: unable to evaluate xpath expression \"%s\"\n",
          current_expr);
      return NULL;
    }

    if (xmlXPathNodeSetGetLength(xpathObj->nodesetval) == 1)
    {
      xpathCtx->node = xpathObj->nodesetval->nodeTab[0];

      result = (struct babel_story_info*)fizmo_malloc(
          sizeof(struct babel_story_info));

      result->release_number = release;
      result->serial = (serial != NULL ? fizmo_strdup(serial) : NULL);
      result->title = getStoryNodeContent(
          xpathCtx, "title", current_namespace);
      result->author = getStoryNodeContent(
          xpathCtx, "author", current_namespace);
      result->description = getStoryNodeContent(
          xpathCtx, "description", current_namespace);
    }
  }

  return result;
}
#else // DISABLE_LIBXML2
struct babel_story_info *get_babel_story_info(uint16_t UNUSED(release),
    char *UNUSED(serial), uint16_t UNUSED(checksum),
    struct babel_info *UNUSED(babel), bool UNUSED(babel_from_blorb))
{
  return NULL;
}
#endif // DISABLE_LIBXML2


void store_babel_info_timestamps(struct babel_info *babel)
{
  FILE *out;
  char *config_dir_name = get_fizmo_config_dir_name();
  char *filename;
  char *quoted_filename;
  int i;

  /*
  Story list should work even if no babel info is available.
  if ( (babel == NULL) || (babel->nof_entries == 0) )
    return;
  */

  if (config_dir_name == NULL)
    return;

  filename = fizmo_malloc(
      strlen(config_dir_name) + 2 + strlen(BABEL_TIMESTAMP_FILE_NAME));

  sprintf(filename, "%s/%s", config_dir_name, BABEL_TIMESTAMP_FILE_NAME);

  if ((out = fopen(filename, "w")) == NULL)
    return;

  if (babel)
  {
    for (i=0; i<babel->nof_entries; i++)
    {
      quoted_filename = quote_special_chars(babel->entries[i]->filename);
      fprintf(out, "%ld\t%s\n", babel->entries[i]->timestamp,
          babel->entries[i]->filename);
      free(quoted_filename);
    }
  }

  fclose(out);
}


void abort_timestamp_input()
{
  if (timestamp_input != NULL)
    free(timestamp_input);
  if (filename_input != NULL)
    free(filename_input);
  if (unquoted_filename_input != NULL)
    free(unquoted_filename_input);
  if (babel_timestamp_entries != NULL)
    free(babel_timestamp_entries);
  fclose(timestamp_file);
}


bool babel_files_have_changed(struct babel_info *babel)
{
  char *config_dir_name = get_fizmo_config_dir_name();
  char *filename;
  int data;
  int nof_babel_timestamp_entries = 0;
  long offset, size;
  long timestamp;
  int i, j;

  if (config_dir_name == NULL)
    return false;

  filename = fizmo_malloc(
      strlen(config_dir_name) + 2 + strlen(BABEL_TIMESTAMP_FILE_NAME));

  //printf("%s/%s", config_dir_name, BABEL_TIMESTAMP_FILE_NAME);
  sprintf(filename, "%s/%s", config_dir_name, BABEL_TIMESTAMP_FILE_NAME);

  if ((timestamp_file = fopen(filename, "r")) == NULL)
    return true;

  if ((data = fgetc(timestamp_file)) != EOF)
  {
    ungetc(data, timestamp_file);
    for(;;)
    {
      offset = ftell(timestamp_file);
      while ((data = fgetc(timestamp_file)) != '\t')
        if (data == EOF)
        {
          abort_timestamp_input();
          return true;
        }

      size = ftell(timestamp_file) - offset - 1;
      if (ensure_mem_size(&timestamp_input, &timestamp_input_size, size + 2)
          == -1)
      {
        abort_timestamp_input();
        return true;
      }

      if (size > 0)
      {
        fseek(timestamp_file, -(size+1), SEEK_CUR);
        if (fread(timestamp_input, size+1, 1, timestamp_file) != 1)
        {
          abort_timestamp_input();
          return true;
        }
      }
      timestamp_input[size] = '\0';

      offset = ftell(timestamp_file);
      while ((data = fgetc(timestamp_file)) != '\n')
        if (data == EOF)
        {
          abort_timestamp_input();
          return true;
        }

      size = ftell(timestamp_file) - offset - 1;
      if (ensure_mem_size(&filename_input, &filename_input_size, size + 2) == -1)
      {
        abort_timestamp_input();
        return true;
      }

      if (size > 0)
      {
        fseek(timestamp_file, -(size+1), SEEK_CUR);
        if (fread(filename_input, size+1, 1, timestamp_file) != 1)
        {
          abort_timestamp_input();
          return true;
        }
      }
      filename_input[size] = '\0';

      //printf("%s,%s\n", timestamp_input, filename_input);

      for (i=0; i<babel->nof_entries; i++)
      {
        if (strcmp(babel->entries[i]->filename, filename_input) == 0)
        {
          //printf("Found %s\n", filename_input);

          timestamp = 0;
          for (j=0; (size_t)j<strlen(timestamp_input); j++)
          {
            if (isdigit(timestamp_input[j]) == 0)
            {
              abort_timestamp_input();
              return true;
            }

            timestamp *= 10;
            timestamp += (timestamp_input[j] - '0');
          }
          //printf("ts:%ld\n", timestamp);

          if (babel->entries[i]->timestamp != timestamp)
          {
            //printf("Timestamps dont match\n");
            abort_timestamp_input();
            return true;
          }

          break;
        }
      }

      if (i == babel->nof_entries)
      {
        abort_timestamp_input();
        //printf("File not found: %s\n", filename_input);
        return true;
      }

      nof_babel_timestamp_entries++;

      if ((data = fgetc(timestamp_file)) == EOF)
        break;
      ungetc(data, timestamp_file);
    }
  }

  abort_timestamp_input();

  if (nof_babel_timestamp_entries != (babel ? babel->nof_entries : 0))
  {
    //printf("Babel-lists have not equal size.\n");
    return true;
  }

  return false;
}


