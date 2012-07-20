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

#include "clint_thread.h"

#if defined(WIN32)

void clint_tls_create(ClintTLS *tls)
{
  tls->key = TlsAlloc();
}

void clint_tls_delete(ClintTLS *tls)
{
  TlsFree(tls->key);
}

void *clint_tls_get(const ClintTLS *tls)
{
  void *value = TlsGetValue(tls->key);
  if (GetLastError() != ERROR_SUCCESS)
    return NULL;
  return value;
}

void clint_tls_set(const ClintTLS *tls, void *value)
{
  TlsSetValue(tls->key, value);
}

void clint_tls_erase(const ClintTLS *tls)
{
  TlsSetValue(tls->key, NULL);
}

#else

void clint_tls_create(ClintTLS *tls)
{
  pthread_key_create(&tls->key, NULL);
}

void clint_tls_delete(ClintTLS *tls)
{
  pthread_key_delete(tls->key);
}

void *clint_tls_get(const ClintTLS *tls)
{
  return pthread_getspecific(tls->key);
}

void clint_tls_set(const ClintTLS *tls, void *value)
{
  pthread_setspecific(tls->key, value);
}

void clint_tls_erase(const ClintTLS *tls)
{
  pthread_setspecific(tls->key, NULL);
}

#endif
