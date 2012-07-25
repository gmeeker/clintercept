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

void* clint_opencl_load(void)
{
  /* Load OpenCL.dll without searching application directory,
     presumably where this library is located. */
  void *lib = LoadLibraryEx(_T("OpenCL.dll"),
                            LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);
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
  _tcscat(path, _T("ClintConfig.txt"));
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
