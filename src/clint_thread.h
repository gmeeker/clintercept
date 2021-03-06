/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
** CLIntercept OpenCL debugging utilities
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

#ifndef _CLINT_THREAD_H_
#define _CLINT_THREAD_H_

#include <stdlib.h>
#include <stdarg.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClintTLS {
#if defined(WIN32)
  DWORD key;
#else
  pthread_key_t key;
#endif
} ClintTLS;

void clint_tls_create(ClintTLS *tls);
void clint_tls_delete(ClintTLS *tls);
void *clint_tls_get(const ClintTLS *tls);
void clint_tls_set(const ClintTLS *tls, void *value);
void clint_tls_erase(const ClintTLS *tls);

#if defined(WIN32)
typedef DWORD ClintProcessId;
typedef DWORD ClintThreadId;
#else
typedef pid_t ClintProcessId;
typedef pthread_t ClintThreadId;
#endif

ClintProcessId clint_get_process_id();
ClintThreadId clint_get_thread_id();

#ifdef __cplusplus
}
#endif

#endif // _CLINT_THREAD_H_
