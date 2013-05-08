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

#include "clint.h"
#include "clint_atomic.h"
#include "clint_config.h"
#include "clint_data.h"
#include "clint_log.h"
#include "clint_obj.h"

#include <ctype.h>
#include <string.h>

#ifdef __APPLE__

void* clint_opencl_load(void)
{
  clint_opencl_init();
  return (void*)1;
}

void* clint_opencl_sym(void *dll, const char *sym)
{
  return NULL;
}

void clint_opencl_unload(void *dll)
{
  clint_opencl_shutdown();
}

#elif defined(WIN32)

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#define LOAD_LIBRARY_SEARCH_USER_DIRS 0x00000400
#endif

void* clint_opencl_load(void)
{
  void *lib;

  /* Load OpenCL.dll without searching application directory,
     presumably where this library is located. */

  if (GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "AddDllDirectory")) {
    lib = LoadLibraryEx(_T("OpenCL.dll"),
                        NULL,
                        LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);
  } else {
    TCHAR path[_MAX_PATH];
    if (SHGetFolderPath(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
      return NULL;
    _tcscat_s(path, _countof(path), _T("\\OpenCL.dll"));
    lib = LoadLibrary(path);
  }
  clint_opencl_init();
  return lib;
}

void* clint_opencl_sym(void *dll, const char *sym)
{
  return (void*)GetProcAddress((HINSTANCE)dll, sym);
}

void clint_opencl_unload(void *dll)
{
  clint_opencl_shutdown();
  FreeLibrary((HINSTANCE)dll);
}

#else

void* clint_opencl_load(void)
{
  void *lib = dlopen("OpenCL.so", RTLD_LAZY | RTLD_LOCAL);
  clint_opencl_init();
  return lib;
}

void* clint_opencl_sym(void *dll, const char *sym)
{
  return dlsym(dll, sym);
}

void clint_opencl_unload(void *dll)
{
  clint_opencl_shutdown();
  dlclose(dll);
}

#endif

void clint_opencl_init()
{
#if defined(WIN32)
  TCHAR path[_MAX_PATH];
  MEMORY_BASIC_INFORMATION mbi;

  clint_data_init();
  if (VirtualQuery(&clint_opencl_init, &mbi, sizeof(mbi)) <= 0)
    return;
  GetModuleFileName((HINSTANCE)mbi.AllocationBase, path, sizeof(path) / sizeof(TCHAR));
  _tcsrchr(path, '\\')[1] = 0; 
  _tcscat_s(path, _countof(path), _T("ClintConfig.txt"));
  clint_config_init(path);
#else
  const char *envstr;
  clint_data_init();
  envstr = getenv("CLINT_CONFIG_FILE");
  if (envstr != NULL) {
    clint_config_init(envstr);
  } else {
#ifdef __APPLE__
    clint_config_init(NULL);
#else
    Dl_info dl_info;
    char path[PATH_MAX];
    dladdr((void*)&clint_opencl_init, &dl_info);
    strcpy(path, dl_info.dli_fname);
    strrchr(path, '/')[1] = 0;
    strcat(path, "ClintConfig.txt");
    clint_config_init(path);
#endif
  }
#endif

  if (clint_get_config(CLINT_LEAKS)) {
    atexit(&clint_log_leaks_all);
  }

  clint_log_describe();

  if (clint_get_config(CLINT_INFO)) {
    /* Don't log internally used calls */
    int enabled = clint_get_config(CLINT_ENABLED);
    clint_set_config(CLINT_ENABLED, 0);
    clint_log_platforms();
    clint_set_config(CLINT_ENABLED, enabled);
  }
}

void clint_opencl_shutdown()
{
  clint_data_shutdown();
  clint_log_shutdown();
}

ClintAtomicInt g_clint_thread_count = 0;

void clint_opencl_enter()
{
  if (CLINT_ATOMIC_ADD(1, g_clint_thread_count) > 1) {
    if (clint_get_config(CLINT_STRICT_THREAD)) {
      clint_log("ERROR: Multiple threads detected.\n");
      clint_log_abort();
    }
  }
}

void clint_opencl_exit()
{
  CLINT_ATOMIC_SUB(1, g_clint_thread_count);
}

static const char *g_clint_extension_name = "cl_CLINT_debugging";
static const char *g_clint_embedded_extensions = "cles_khr_int64 cl_CLINT_debugging";

void clint_extensions_modify(size_t len, char *s, size_t *ret_ptr)
{
  size_t ret;

  if (clint_get_config(CLINT_ENABLED)) {
    const char *new_exts = g_clint_extension_name;

    if (clint_get_config(CLINT_EMBEDDED)) {
      new_exts = g_clint_embedded_extensions;
    }

    /* Add clint extension to the string. */
    if (ret_ptr != NULL) {
      ret = *ret_ptr;
    } else {
      ret = strlen(s) + 1;
    }
    if (ret > 0) {
      ret++;
    }
    ret += strlen(new_exts);
    if (ret_ptr != NULL) {
      *ret_ptr = ret;
    }
    if (s != NULL && len >= ret) {
      size_t slen = strlen(s);
      if (slen > 0 && slen+1 < len) {
        s[slen++] = ' ';
      }
#if defined(WIN32)
      strcpy_s(s + slen, len - slen, new_exts);
#else
      strcpy(s + slen, new_exts);
#endif
      slen += strlen(new_exts);
      if (slen >= len) {
        slen = len-1;
      }
      s[slen] = 0;

      /* Filter any disabled extensions. */
      /* To avoid filtering during length queries, we pad but don't adjust the length. */
      if (clint_get_config(CLINT_DISABLE_EXTENSION)) {
        const char *exts_src = clint_get_config_string(CLINT_DISABLE_EXTENSION);
        if (exts_src != NULL) {
          char *exts;
          char *match;
          if ((exts = strdup(exts_src)) != NULL) {
            size_t i, j;
            for (i = 0; exts[i] != 0; i++) {
              while (isspace(exts[i]))
                i++;
              for (j = i; exts[j] != 0 && !isspace(exts[j]); j++) {
              }
              exts[j] = 0;
              match = strstr(s, exts+i);
              while (match != NULL && match[j-i] != 0 && !isspace(match[j-i])) {
                /* We found a longer extension.  Keep looking. */
                match = strstr(match+1, exts+i);
              }
              if (match != NULL) {
                const char *next = match + j - i;
                while (*next != 0 && isspace(*next)) {
                  next++;
                }
                /* The strings may overlay so use memmove not strcpy. */
                memmove(match, next, s + len - next);
                /* And zero the extra space. */
                memset(s + len - (next - match), 0, next - match);
              }
              i = j;
            }
            free(exts);
          }
        }
      }
    }
  }
}
