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

#include "clint_data.h"
#include "clint_thread.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static ClintTLS g_clint_autopool_key;
static int g_clint_autopool_init = 0;

void clint_data_init()
{
  if (g_clint_autopool_init == 0) {
    g_clint_autopool_init = 1;
    clint_tls_create(&g_clint_autopool_key);
  }
}

void clint_data_shutdown()
{
  if (g_clint_autopool_init == 1) {
    g_clint_autopool_init = 0;
    clint_tls_delete(&g_clint_autopool_key);
  }
}

void *clint_autopool_malloc(size_t size)
{
  ClintAutopool *pool;

  pool = clint_tls_get(&g_clint_autopool_key);
  if (pool == NULL) {
    /* Leaking memory... */
    return malloc(size);
  } else {
    ClintAutopoolElem *elem = (ClintAutopoolElem*)malloc(size + sizeof(ClintAutopoolElem));
    CLINT_STACK_PUSH(pool->ptr, elem);
    return elem + 1;
  }
}

void clint_autopool_begin(ClintAutopool *pool)
{
  ClintAutopool *pools;

  clint_data_init();
  pools = clint_tls_get(&g_clint_autopool_key);
  pool->ptr = NULL;
  CLINT_STACK_PUSH(pools, pool);
  clint_tls_set(&g_clint_autopool_key, pools);
}

void clint_autopool_end(ClintAutopool *pool)
{
  ClintAutopool *pools;

  pools = clint_tls_get(&g_clint_autopool_key);
  assert(pools == pool);
  pool = CLINT_STACK_POP(pools);
  clint_tls_set(&g_clint_autopool_key, pools);

  CLINT_STACK_ITER(pool->ptr, free);
}

const char *clint_string_vsprintf(const char *fmt, va_list ap)
{
  size_t size;
  char *buf;
  char tmp[8];
  va_list ap2;

  va_copy(ap2, ap);
#if defined(WIN32)
  size = (size_t)vsprintf_s(tmp, sizeof(tmp), fmt, ap2) + 1;
#else
  size = (size_t)vsnprintf(tmp, sizeof(tmp), fmt, ap2) + 1;
#endif
  va_end(ap2);
  buf = (char*)clint_autopool_malloc(size);
#if defined(WIN32)
  vsprintf_s(buf, size, fmt, ap);
#else
  vsnprintf(buf, size, fmt, ap);
#endif

  return buf;
}

const char *clint_string_sprintf(const char *fmt, ...)
{
  va_list ap;
  const char *ret;

  va_start(ap, fmt);
  ret = clint_string_vsprintf(fmt, ap);
  va_end(ap);

  return ret;
}

const char *clint_string_cat(const char *s1, const char *s2)
{
  char *buf;
  size_t size;

  if (s1 == NULL || *s1 == 0)
    return s2;
  if (s2 == NULL || *s2 == 0)
    return s1;
  size = strlen(s1) + strlen(s2) + 1;
  buf = (char*)clint_autopool_malloc(size);
  strncpy(buf, s1, size);
  strncat(buf, s2, size);

  return buf;
}

const char *clint_string_join(const char *s1, const char *s2, const char *j)
{
  char *buf;
  size_t size;

  if (j == NULL || *j == 0)
    return clint_string_cat(s1, s2);
  if (s1 == NULL || *s1 == 0)
    return s2;
  if (s2 == NULL || *s2 == 0)
    return s1;
  size = strlen(s1) + strlen(s2) + strlen(j) + 1;
  buf = (char*)clint_autopool_malloc(size);
  strncpy(buf, s1, size);
  strncat(buf, j, size);
  strncat(buf, s2, size);

  return buf;
}
