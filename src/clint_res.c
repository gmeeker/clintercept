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

#include "clint_res.h"
#include "clint_atomic.h"
#include "clint_config.h"
#include "clint_data.h"
#include "clint_stack.h"
#include "clint_tree.h"

#define CLINT_IMPL_RES_FUNCS(type)                                  \
typedef struct ClintResource_##type {                               \
  CLINT_TREE_ELEMS(struct ClintResource_##type, cl_##type);         \
  char *stack;                                                      \
  ClintAtomicInt refCount;                                          \
} ClintResource_##type;                                             \
                                                                    \
CLINT_DEFINE_TREE_FUNCS(ClintResource_##type, cl_##type);           \
CLINT_IMPL_TREE_FUNCS(ClintResource_##type, cl_##type);             \
                                                                    \
static ClintResource_##type *g_clint_resources_##type;              \
static ClintSpinLock g_clint_lock_##type;                           \
                                                                    \
static ClintResource_##type *clint_lookup_##type(cl_##type v)       \
{                                                                   \
  ClintResource_##type *res = NULL;                                 \
  if (!clint_get_config(CLINT_TRACK))                               \
    return NULL;                                                    \
  CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                         \
  res = clint_tree_find_ClintResource_##type(g_clint_resources_##type, v); \
  CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                       \
  if (res == NULL) {                                                \
    clint_log("ERROR: Unknown cl_" #type " %p\n", v);               \
    clint_log_abort();                                              \
  }                                                                 \
  if (res->refCount <= 0) {                                         \
    clint_log("ERROR: cl_" #type " %p was previously freed.\n", v); \
    if (res->stack) {                                               \
      clint_log("Allocated at:\n%s\n", res->stack);                 \
    }                                                               \
    clint_log_abort();                                              \
  }                                                                 \
  return res;                                                       \
}                                                                   \
                                                                    \
void clint_check_input_##type(cl_##type v)                          \
{                                                                   \
  (void)clint_lookup_##type(v);                                     \
}                                                                   \
                                                                    \
void clint_check_output_##type(cl_##type v)                         \
{                                                                   \
  if (clint_get_config(CLINT_TRACK)) {                              \
    ClintResource_##type *res =                                     \
      (ClintResource_##type*)malloc(sizeof(ClintResource_##type));  \
    res->stack = NULL;                                              \
    res->refCount = 1;                                              \
    if (clint_get_config(CLINT_STACK_LOGGING)) {                    \
      res->stack = clint_get_stack();                               \
    }                                                               \
    CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                       \
    clint_tree_insert_ClintResource_##type(&g_clint_resources_##type, v, res); \
    CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                     \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_check_input_##type##s(cl_uint num, const cl_##type *v)   \
{                                                                   \
  if (v != NULL) {                                                  \
    cl_uint i = 0;                                                  \
    for (i = 0; i < num; i++) {                                     \
      clint_check_input_##type(v[i]);                               \
    }                                                               \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_check_output_##type##s(cl_uint num, cl_##type *v)        \
{                                                                   \
  if (v != NULL) {                                                  \
    cl_uint i = 0;                                                  \
    for (i = 0; i < num; i++) {                                     \
      clint_check_output_##type(v[i]);                              \
    }                                                               \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_retain_##type(cl_##type v)                               \
{                                                                   \
  ClintResource_##type *res = clint_lookup_##type(v);               \
  if (res != NULL) {                                                \
    CLINT_ATOMIC_ADD(1, res->refCount);                             \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_release_##type(cl_##type v)                              \
{                                                                   \
  ClintResource_##type *res = clint_lookup_##type(v);               \
  if (res != NULL) {                                                \
    ClintAtomicInt count = CLINT_ATOMIC_SUB(1, res->refCount);      \
    if (count == 0 &&                                               \
        !clint_get_config(CLINT_ZOMBIES)) {                         \
      CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                     \
      clint_tree_erase_ClintResource_##type(&g_clint_resources_##type, res); \
      if (res->stack) {                                             \
        free(res->stack);                                           \
      }                                                             \
      free(res);                                                    \
      CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                   \
    }                                                               \
  }                                                                 \
}                                                                   \

CLINT_IMPL_RES_FUNCS(context);
CLINT_IMPL_RES_FUNCS(command_queue);
CLINT_IMPL_RES_FUNCS(mem);
CLINT_IMPL_RES_FUNCS(program);
CLINT_IMPL_RES_FUNCS(kernel);
CLINT_IMPL_RES_FUNCS(event);
CLINT_IMPL_RES_FUNCS(sampler);

void clint_kernel_enter(cl_kernel kernel)
{
}

void clint_kernel_exit(cl_kernel kernel)
{
}
