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

#include "clint_stack.h"

#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#else
#include <execinfo.h>
#endif

char *clint_get_stack()
{
#if defined(WIN32)
  SYMBOL_INFO *symbol;
  HANDLE process;
  void *frames[128];
  char *buf;
  size_t size, bufi;
  int i, count;
  char tmp[8];

  process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);

  count = CaptureStackBackTrace(0, sizeof(frames) / sizeof(void*), frames, NULL);
  symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  size = 0;
  for (i = 0; i < count; ++i) {
    SymFromAddr(process, (DWORD64)(frames[i]), 0, symbol);
    size += sprintf_s(tmp, sizeof(tmp), "%i: %s - 0x%0X\n", count - i - 1, symbol->Name, symbol->Address);
  }
  buf = malloc(size);
  bufi = 0;
  for (i = 0; i < count; ++i) {
    bufi += sprintf_s(buf + bufi, size - bufi, "%i: %s - 0x%0X\n", count - i - 1, symbol->Name, symbol->Address);
  }
  free(symbol);

  return buf;
#else
  void *frames[128];
  char **strs;
  char *buf;
  size_t size, bufi, strl;
  int i, count;

  count = backtrace(frames, sizeof(frames) / sizeof(void*));
  strs = backtrace_symbols(frames, count);
  size = 0;
  for (i = 0; i < count; ++i) {
    size += strlen(strs[i]) + 1;
  }
  buf = malloc(size);
  bufi = 0;
  for (i = 0; i < count; ++i) {
    strl = strlen(strs[i]);
    strncpy(buf + bufi, strs[i], strl);
    bufi += strl;
    buf[bufi++] = '\n';
  }
  buf[bufi++] = 0;
  free(strs);

  return buf;
#endif
}
