/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
** Clint OpenCL debugging utilities
** Copyright (c) 2012, Digital Anarchy, Inc.
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice,
**   this list of conditions and the following disclaimer in the documentation
**   and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "clint_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETENV
#define HAVE_GETENV (!defined(WIN32))
#endif

static const char *g_clint_config_names[CLINT_MAX] = {
  "CLINT_ENABLED",
  "CLINT_CONFIG_FILE",
  "CLINT_LOG_FILE",
  "CLINT_TRACE",
  "CLINT_TRACK",
  "CLINT_CHECK_REFS",
  "CLINT_CHECK_THREAD",
  "CLINT_STRICT_THREAD",
  "CLINT_CHECK_ALL",
  "CLINT_ABORT",
  NULL
};

static int g_clint_config_values[CLINT_MAX];

void clint_config_init(const ClintPathChar *path)
{
  int i;
#if HAVE_GETENV
  const char *envstr;
  envstr = getenv(g_clint_config_names[CLINT_CONFIG_FILE]);
  if (envstr)
    path = envstr;
#endif

  if (path) {
    FILE *fp;
#if defined(WIN32)
    fp = _tfopen(path, _T("r"));
#else
    fp = fopen(path, "r");
#endif
    if (fp != NULL) {
      size_t bufsize = 2048;
      char *buf = (char*)malloc(bufsize);
      char *key = (char*)malloc(bufsize);
      while (fgets(buf, bufsize, fp) != NULL) {
        if (sscanf(buf, " %s ", key) == 1) {
          const char *s = strchr(buf, '=');
          if (s) {
            s++;
            for (i = 0; i < CLINT_MAX; i++) {
              if (strcmp(g_clint_config_names[i], key) == 0) {
                g_clint_config_values[i] = atoi(s);
              }
            }
          }
        }
      }
      free(buf);
      fclose(fp);
    }
  }

#if HAVE_GETENV
  for (i = 0; i < CLINT_MAX; i++) {
    const char *envstr = getenv(g_clint_config_names[i]);
    if (envstr && *envstr) {
      g_clint_config_values[i] = atoi(envstr);
    }
  }
#endif
}

int clint_get_config(ClintConfig v)
{
  return g_clint_config_values[v];
}
