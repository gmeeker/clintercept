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

#include "clint_obj.h"
#include "clint_config.h"
#include "clint_data.h"
#include "clint_stack.h"

#define CLINT_IMPL_OBJ_FUNCS(type)                                  \
                                                                    \
CLINT_DEFINE_TREE_FUNCS(ClintObject_##type, cl_##type);             \
CLINT_IMPL_TREE_FUNCS(ClintObject_##type, cl_##type);               \
                                                                    \
static ClintObject_##type *g_clint_objects_##type;                  \
static ClintSpinLock g_clint_lock_##type;                           \
                                                                    \
ClintObject_##type *clint_lookup_##type(cl_##type v)                \
{                                                                   \
  ClintObject_##type *obj = NULL;                                   \
  if (!clint_get_config(CLINT_TRACK))                               \
    return NULL;                                                    \
  CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                         \
  obj = clint_tree_find_ClintObject_##type(g_clint_objects_##type, v); \
  CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                       \
  if (obj == NULL) {                                                \
    clint_log("ERROR: Unknown cl_" #type " %p\n", v);               \
    clint_log_abort();                                              \
  }                                                                 \
  if (obj->refCount <= 0) {                                         \
    clint_log("ERROR: cl_" #type " %p was previously freed.\n", v); \
    if (obj->stack) {                                               \
      clint_log("Allocated at:\n%s\n", obj->stack);                 \
    }                                                               \
    clint_log_abort();                                              \
  }                                                                 \
  return obj;                                                       \
}                                                                   \
                                                                    \
void clint_check_input_##type(cl_##type v)                          \
{                                                                   \
  (void)clint_lookup_##type(v);                                     \
}                                                                   \
                                                                    \
void clint_check_output_##type(cl_##type v, void *src, ClintObjType t ARGS) \
{                                                                   \
  if (clint_get_config(CLINT_TRACK)) {                              \
    ClintObject_##type *obj =                                       \
      (ClintObject_##type*)malloc(sizeof(ClintObject_##type));      \
    obj->stack = NULL;                                              \
    switch (t) {                                                    \
    case ClintObjectType_context:                                   \
      obj->context = (cl_context)src;                               \
      break;                                                        \
    case ClintObjectType_command_queue:                             \
      obj->context = clint_lookup_command_queue((cl_command_queue)src)->context; \
      break;                                                        \
    case ClintObjectType_mem:                                       \
    case ClintObjectType_sub_bufer:                                 \
    case ClintObjectType_image2d:                                   \
    case ClintObjectType_image3d:                                   \
      obj->context = clint_lookup_mem((cl_mem)src)->context;        \
      break;                                                        \
    case ClintObjectType_program:                                   \
      obj->context = clint_lookup_program((cl_program)src)->context;\
      break;                                                        \
    case ClintObjectType_kernel:                                    \
      obj->context = clint_lookup_kernel((cl_kernel)src)->context;  \
      break;                                                        \
    case ClintObjectType_event:                                     \
      obj->context = clint_lookup_event((cl_event)src)->context;    \
      break;                                                        \
    case ClintObjectType_sampler:                                   \
      obj->context = clint_lookup_sampler((cl_sampler)src)->context;\
      break;                                                        \
    case ClintObjectType_device:                                    \
      obj->context = clint_lookup_device_id((cl_device_id)src)->context; \
      break;                                                        \
    default:                                                        \
      obj->context = NULL;                                          \
      break;                                                        \
    }                                                               \
    obj->refCount = 1;                                              \
    if (clint_get_config(CLINT_STACK_LOGGING)) {                    \
      obj->stack = clint_get_stack();                               \
    }                                                               \
    CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                       \
    clint_tree_insert_ClintObject_##type(&g_clint_objects_##type, v, obj); \
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
void clint_check_output_##type##s(cl_uint num, cl_##type *v, void *src, ClintObjType t ARGS) \
{                                                                   \
  if (v != NULL) {                                                  \
    cl_uint i = 0;                                                  \
    for (i = 0; i < num; i++) {                                     \
      clint_check_output_##type(v[i], src, t ARGNAMES);             \
    }                                                               \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_retain_##type(cl_##type v)                               \
{                                                                   \
  ClintObject_##type *obj = clint_lookup_##type(v);                 \
  if (obj != NULL) {                                                \
    CLINT_ATOMIC_ADD(1, obj->refCount);                             \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_release_##type(cl_##type v)                              \
{                                                                   \
  ClintObject_##type *obj = clint_lookup_##type(v);                 \
  if (obj != NULL) {                                                \
    ClintAtomicInt count = CLINT_ATOMIC_SUB(1, obj->refCount);      \
    if (count == 0 &&                                               \
        !clint_get_config(CLINT_ZOMBIES)) {                         \
      CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                     \
      clint_tree_erase_ClintObject_##type(&g_clint_objects_##type, obj); \
      if (obj->stack) {                                             \
        free(obj->stack);                                           \
      }                                                             \
      free(obj);                                                    \
      CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                   \
    }                                                               \
  }                                                                 \
}                                                                   \
                                                                    \
static void clint_log_leaks_##type(ClintObject_##type *tree, cl_context context) \
{                                                                   \
  ClintObject_##type *iter = clint_tree_first_ClintObject_##type(tree); \
  while (iter) {                                                    \
    if (context == NULL || context == iter->context) {              \
      clint_log("Possibly leaked cl_" #type ": %p\n", iter->_key);  \
      if (iter->stack != NULL)                                      \
        clint_log("Created at:\n%s\n", iter->stack);                \
      iter = clint_tree_next_ClintObject_##type(iter);              \
    }                                                               \
  }                                                                 \
}                                                                   \

#define ARGS
#define ARGNAMES
CLINT_IMPL_OBJ_FUNCS(context);
CLINT_IMPL_OBJ_FUNCS(command_queue);
#undef ARGS
#undef ARGNAMES
#define ARGS , cl_mem_flags flags, ClintObjSharing sharing
#define ARGNAMES , flags, sharing
CLINT_IMPL_OBJ_FUNCS(mem);
#undef ARGS
#undef ARGNAMES
#define ARGS
#define ARGNAMES
CLINT_IMPL_OBJ_FUNCS(program);
CLINT_IMPL_OBJ_FUNCS(kernel);
CLINT_IMPL_OBJ_FUNCS(event);
CLINT_IMPL_OBJ_FUNCS(sampler);
CLINT_IMPL_OBJ_FUNCS(device_id);

void clint_acquire_shared_mem(cl_mem v, ClintObjSharing sharing)
{
}

void clint_acquire_shared_mems(cl_uint num, const cl_mem *v, ClintObjSharing sharing)
{
}

void clint_release_shared_mem(cl_mem v, ClintObjSharing sharing)
{
}

void clint_release_shared_mems(cl_uint num, const cl_mem *v, ClintObjSharing sharing)
{
}


void clint_kernel_enter(cl_kernel kernel)
{
  if (clint_get_config(CLINT_CHECK_THREAD)) {
    ClintObject_kernel *obj = clint_lookup_kernel(kernel);
    if (obj != NULL) {
      if (CLINT_ATOMIC_ADD(1, obj->threadCount) > 1) {
        clint_log("ERROR: Multiple threads detected modifying the kernel %p.\n", kernel);
        clint_log_abort();
      }
    }
  }
}

void clint_kernel_exit(cl_kernel kernel)
{
  if (clint_get_config(CLINT_CHECK_THREAD)) {
    ClintObject_kernel *obj = clint_lookup_kernel(kernel);
    if (obj != NULL) {
      CLINT_ATOMIC_SUB(1, obj->threadCount);
    }
  }
}

void clint_log_leaks(cl_context context)
{
  if (context == NULL)
    clint_log("Possible leaked OpenCL objects:\n");
  else
    clint_log("Possible leaked OpenCL objects for cl_context %p:\n", context);
  if (context == NULL)
    clint_log_leaks_context(g_clint_objects_context, NULL);
  clint_log_leaks_command_queue(g_clint_objects_command_queue, context);
  clint_log_leaks_mem(g_clint_objects_mem, context);
  clint_log_leaks_program(g_clint_objects_program, context);
  clint_log_leaks_kernel(g_clint_objects_kernel, context);
  clint_log_leaks_event(g_clint_objects_event, context);
  clint_log_leaks_sampler(g_clint_objects_sampler, context);
  clint_log_leaks_device_id(g_clint_objects_device_id, context);
}

void clint_log_leaks_all(void)
{
  clint_log_leaks(NULL);
}
