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

#include "clint_mem.h"
#include <string.h>

#if defined(WIN32)

#include <windows.h>

static size_t clint_pagesize(void)
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (size_t)info.dwPageSize;
}

int clint_mem_alloc(struct ClintMem *mem, size_t bytes, unsigned int flags)
{
  size_t p = clint_pagesize();
  size_t memsize = (bytes + p - 1) & ~(p - 1);
  mem->size = bytes;
  mem->size_page = memsize;
  if ((flags & ClintMemProtection_Guard_Before) != 0)
    memsize += p;
  if ((flags & ClintMemProtection_Guard_After) != 0)
    memsize += p;
  mem->size_real = memsize;
  mem->addr = VirtualAlloc(NULL, memsize, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
  mem->addr_real = mem->addr_page = mem->addr;
  if ((flags & ClintMemProtection_Guard_Before) != 0) {
    mem->addr = (char*)mem->addr + p;
    if (VirtualProtect(mem->addr_real, p, PAGE_NOACCESS, NULL) == 0)
      return 1;
  }
  if ((flags & ClintMemProtection_Guard_After) != 0) {
    if (VirtualProtect((char*)mem->addr + mem->size_page, p, PAGE_NOACCESS, NULL) == 0)
      return 1;
    if ((flags & ClintMemProtection_Guard_Before) == 0) {
      /* Move pointer to end at the page (allowing for SIMD alignment). */
      size_t pad = mem->size_page - mem->size;
      size_t align = 32;
      pad &= ~align;
      mem->addr = (char*)mem->addr + pad;
    }
  }
  return clint_mem_protect(mem, flags);
}

int clint_mem_protect(struct ClintMem *mem, unsigned int flags)
{
  DWORD prot = PAGE_NOACCESS;
  if ((flags & ClintMemProtection_Execute) != 0) {
    if ((flags & ClintMemProtection_Write) != 0)
      prot = PAGE_EXECUTE_READWRITE;
    else
      prot = PAGE_EXECUTE_READ;
  } else {
    if ((flags & ClintMemProtection_Write) != 0)
      prot = PAGE_READWRITE;
    else
      prot = PAGE_READONLY;
  }
  if (VirtualProtect(mem->addr_page, mem->size_page, prot, NULL) == 0)
    return 1;
  return 0;
}

int clint_mem_free(struct ClintMem *mem)
{
  if (mem->addr_real != NULL) {
    VirtualFree(mem->addr_real, 0, MEM_RELEASE);
    memset(mem, 0, sizeof(*mem));
  }
  return 0;
}

#else

#include <sys/mman.h>
#include <unistd.h>

static size_t clint_pagesize(void)
{
  return sysconf(_SC_PAGESIZE);
}

int clint_mem_alloc(struct ClintMem *mem, size_t bytes, unsigned int flags)
{
  size_t p = clint_pagesize();
  size_t memsize = (bytes + p - 1) & ~(p - 1);
  mem->size = bytes;
  mem->size_page = memsize;
  if ((flags & ClintMemProtection_Guard_Before) != 0)
    memsize += p;
  if ((flags & ClintMemProtection_Guard_After) != 0)
    memsize += p;
  mem->size_real = memsize;
#ifdef MAP_ANON
  mem->addr = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
  if (mem->addr == MAP_FAILED) {
    return 1;
  }
#else
  mem->fd = open("/dev/zero", O_RDWR);
  if (mem->fd == -1) {
    return 1;
  }
  mem->addr = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, mem->fd, 0);
  if (mem->addr == MAP_FAILED) {
    close(mem->fd);
    mem->fd = -1;
    return 1;
  }
#endif
  mem->addr_real = mem->addr_page = mem->addr;
  if ((flags & ClintMemProtection_Guard_Before) != 0) {
    mem->addr = (char*)mem->addr + p;
    if (mprotect(mem->addr_real, p, PROT_NONE) != 0)
      return 1;
  }
  if ((flags & ClintMemProtection_Guard_After) != 0) {
    if (mprotect((char*)mem->addr + mem->size_page, p, PROT_NONE) != 0) {
      return 1;
    }
    if ((flags & ClintMemProtection_Guard_Before) == 0) {
      /* Move pointer to end at the page (allowing for SIMD alignment). */
      size_t pad = mem->size_page - mem->size;
      size_t align = 32;
      pad &= ~align;
      mem->addr = (char*)mem->addr + pad;
    }
  }
  return clint_mem_protect(mem, flags);
}

int clint_mem_protect(struct ClintMem *mem, unsigned int flags)
{
  int prot = PROT_NONE;
  if ((flags & ClintMemProtection_Read) != 0)
    prot |= PROT_READ;
  if ((flags & ClintMemProtection_Write) != 0)
    prot |= PROT_WRITE;
  if ((flags & ClintMemProtection_Execute) != 0)
    prot |= PROT_EXEC;
  if (mprotect(mem->addr_page, mem->size_page, prot) != 0)
    return 1;
  return 0;
}

int clint_mem_free(struct ClintMem *mem)
{
  if (mem->addr_real != NULL) {
    munmap(mem->addr_real, mem->size_real);
#ifndef MAP_ANON
    if (mem->fd != -1) {
      close(mem->fd);
      mem->fd = -1;
    }
#endif
    memset(mem, 0, sizeof(*mem));
  }
  return 0;
}

#endif
