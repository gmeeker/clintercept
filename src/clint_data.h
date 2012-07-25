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

#ifndef _CLINT_DATA_H_
#define _CLINT_DATA_H_

#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClintAutopoolElem {
  struct ClintAutopoolElem *next;
} ClintAutopoolElem;

typedef struct ClintAutopool {
  struct ClintAutopool *next;
  struct ClintAutopoolElem *ptr;
} ClintAutopool;

#define CLINT_STACK_PUSH(LIST, ELEM)            \
  (ELEM)->next = (LIST);                        \
  (LIST) = (ELEM)
#define CLINT_STACK_POP(LIST)                   \
  ({                                            \
    typeof(*(LIST)) *_ptr = (LIST);             \
    if ((LIST))                                 \
      (LIST) = (LIST)->next;                    \
    _ptr;                                       \
  })
#define CLINT_STACK_ITER(LIST, FUNC)            \
  {                                             \
    typeof(*(LIST)) *_ptr = (LIST);             \
    while (_ptr) {                              \
      typeof(*(LIST)) *_ptrnext = _ptr->next;   \
      (FUNC)(_ptr);                             \
      _ptr = _ptrnext;                          \
    }                                           \
  }

#define CLINT_LIST_ELEMS(T, KEY)                \
  T *prev;                                      \
  T *next;                                      \
  KEY key;
#define CLINT_LIST_INSERT(LIST, KEY, ELEM)      \
  (ELEM)->next = (LIST);                        \
  (ELEM)->prev = NULL;                          \
  (ELEM)->key = (KEY);                          \
  if ((LIST))                                   \
    (LIST)->prev = (ELEM);                      \
  (LIST) = (ELEM)
#define CLINT_LIST_REMOVE(LIST, ELEM)           \
  {                                             \
    typeof((LIST)) _ptr = (LIST);               \
    while (_ptr) {                              \
      if (_ptr == (ELEM)) {                     \
        if (_ptr->prev) {                       \
          _ptr->prev->next = _ptr->next;        \
        }                                       \
        if (_ptr->next) {                       \
          _ptr->next->prev = _ptr->prev;        \
        }                                       \
        break;                                  \
      }                                         \
      _ptr = _ptr->next;                        \
    }                                           \
  }
#define CLINT_LIST_FIND(LIST, KEY)              \
  ({                                            \
    typeof((LIST)) _ptr = (LIST);               \
    while (_ptr) {                              \
      if (_ptr->key == (KEY))                   \
        break;                                  \
      _ptr = _ptr->next;                        \
    }                                           \
    _ptr;                                       \
  })

void clint_data_init();
void clint_data_shutdown();
void *clint_autopool_malloc(size_t size);
void clint_autopool_begin(ClintAutopool*);
void clint_autopool_end(ClintAutopool*);
const char *clint_string_shorten(const char *s);
const char *clint_string_vsprintf(const char *fmt, va_list ap);
const char *clint_string_sprintf(const char *fmt, ...);
const char *clint_string_cat(const char *s1, const char *s2);
const char *clint_string_join(const char *s1, const char *s2, const char *j);

#ifdef __cplusplus
}
#endif

#endif // _CLINT_DATA_H_
