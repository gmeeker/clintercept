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
#include "clint_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#ifndef HAVE_GETENV
#define HAVE_GETENV (!defined(WIN32))
#endif

#if defined(WIN32)
#define strcasecmp stricmp
#endif

static const char *g_clint_config_names[CLINT_MAX] = {
  "CLINT_ENABLED",
  "CLINT_CONFIG_FILE",
  "CLINT_LOG_FILE",
  "CLINT_TRACE",
  "CLINT_ERRORS",
  "CLINT_TRACK",
  "CLINT_ZOMBIES",
  "CLINT_LEAKS",
  "CLINT_STACK_LOGGING",
  "CLINT_CHECK_THREAD",
  "CLINT_STRICT_THREAD",
  "CLINT_CHECK_ALL",
  "CLINT_ABORT",
  "CLINT_INFO",
  "CLINT_PROFILE",
  "CLINT_PROFILE_ALL"
};

static int g_clint_config_values[CLINT_MAX];

static const char *g_clint_config_describe[CLINT_MAX] = {
  "CLINT_ENABLED enabled.\n",
  "CLINT_CONFIG_FILE enabled:\n",
  "CLINT_LOG_FILE enabled:\n",
  "CLINT_TRACE enabled: logging all OpenCL calls.\n",
  "CLINT_ERRORS enabled: logging all OpenCL errors.\n",
  "CLINT_TRACK enabled: track all OpenCL resources.\n",
  "CLINT_ZOMBIES enabled: remember deallocated resources.\n",
  "CLINT_LEAKS enabled: report any leaked resources.\n",
  "CLINT_STACK_LOGGING enabled: log stack during resource allocation.\n",
  "CLINT_CHECK_THREAD enabled: check for illegal concurrent calls.\n",
  "CLINT_STRICT_THREAD enabled: check for any concurrent calls.\n",
  "CLINT_CHECK_ALL enabled: full OpenCL checking.\n",
  "CLINT_ABORT enabled: break on OpenCL errors.\n",
  "CLINT_INFO enabled: show device capabilities.\n",
  "CLINT_PROFILE enabled: profile kernel execution.\n",
  "CLINT_PROFILE_ALL enabled: profile OpenCL calls.\n"
};

static int clint_config_parse_flag(const char *s)
{
  char *end;
  long l;

  while (isspace(*s))
    s++;
  if (strcasecmp(s, "true") == 0)
    return 1;
  if (strcasecmp(s, "false") == 0)
    return 0;
  if (strcasecmp(s, "yes") == 0)
    return 1;
  if (strcasecmp(s, "no") == 0)
    return 0;
  if (strcasecmp(s, "on") == 0)
    return 1;
  if (strcasecmp(s, "off") == 0)
    return 0;
  l = strtol(s, &end, 0);
  if (end != s)
    return (int)l;
  return 1;
}

static int clint_config_parse_string(char *output, const char *input)
{
  if (input == NULL || *input == 0)
    return 0;
  while (isspace(*input))
    input++;
  if (*input++ != '"')
    return 0;
  while (*input != '"') {
    if (*input == 0)
      return 0;
    if (*input == '\\')
      input++;
    if (*input == 0)
      return 0;
    *output++ = *input++;
  }
  return 1;
}

void clint_config_init(const ClintPathChar *path)
{
  size_t bufsize = 2048;
  char *buf = (char*)malloc(bufsize);
  char *key = (char*)malloc(bufsize);
  char *logfile = (char*)malloc(bufsize);
  const char *logfile_ptr = NULL;
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
      while (fgets(buf, bufsize, fp) != NULL) {
        if (sscanf(buf, " %s ", key) == 1) {
          const char *s = strchr(buf, '=');
          if (s) {
            s++;
            for (i = 0; i < CLINT_MAX; i++) {
              if (strcasecmp(key, g_clint_config_names[i]) == 0 ||
                  strcasecmp(key, g_clint_config_names[i] + 6 /* without CLINT_ */) == 0) {
                switch (i) {
                case CLINT_CONFIG_FILE:
                  break;
                case CLINT_LOG_FILE:
                  if (clint_config_parse_string(logfile, s)) {
                    logfile_ptr = logfile;
                  }
                  break;
                default:
                  clint_set_config(i, clint_config_parse_flag(s));
                  break;
                }
              }
            }
          }
        }
      }
      fclose(fp);
    }
  }

#if HAVE_GETENV
  for (i = 0; i < CLINT_MAX; i++) {
    const char *envstr = getenv(g_clint_config_names[i]);
    if (envstr) {
      switch (i) {
      case CLINT_CONFIG_FILE:
        break;
      case CLINT_LOG_FILE:
        logfile_ptr = envstr;
        break;
      default:
        clint_set_config(i, clint_config_parse_flag(envstr));
        break;
      }
    }
  }
#endif

  if (logfile_ptr != NULL) {
    if (strcmp(logfile_ptr, "stdout") == 0 ||
        strcmp(logfile_ptr, "-") == 0 ||
        strcmp(logfile_ptr, "1") == 0) {
      clint_log_init_fp(stdout);
    } else if (strcmp(logfile_ptr, "stderr") == 0 ||
               strcmp(logfile_ptr, "2") == 0) {
      clint_log_init_fp(stderr);
    } else {
      clint_log_init(logfile_ptr);
    }
  }

  free(buf);
  free(key);
  free(logfile);

  /* Finally set any implied config items. */
  for (i = 0; i < CLINT_MAX; i++) {
    if (i != CLINT_ENABLED) {
      if (g_clint_config_values[i]) {
        clint_set_config(CLINT_ENABLED, 1);
        break;
      }
    }
  }
  if (clint_get_config(CLINT_TRACE)) {
    clint_set_config(CLINT_ERRORS, 1);
  }
  if (clint_get_config(CLINT_STRICT_THREAD)) {
    clint_set_config(CLINT_CHECK_THREAD, 1);
  }
  if (clint_get_config(CLINT_CHECK_ALL)) {
    clint_set_config(CLINT_TRACK, 1);
    clint_set_config(CLINT_CHECK_THREAD, 1);
  }
  if (clint_get_config(CLINT_PROFILE_ALL)) {
    clint_set_config(CLINT_PROFILE, 1);
  }
}

int clint_get_config(ClintConfig which)
{
  if (!g_clint_config_values[CLINT_ENABLED])
    return 0;
  return g_clint_config_values[which];
}

void clint_set_config(ClintConfig which, int v)
{
  g_clint_config_values[which] = v;
}

const char *clint_config_describe(ClintConfig which)
{
  return g_clint_config_describe[which];
}
