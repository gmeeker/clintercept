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

#ifndef _CLINT_OBJ_H_
#define _CLINT_OBJ_H_

#include "clint_atomic.h"
#include "clint_log.h"
#include "clint_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ClintObjType {
  ClintObjectType_none,
  ClintObjectType_context,
  ClintObjectType_command_queue,
  ClintObjectType_mem,
  ClintObjectType_sub_bufer,
  ClintObjectType_image2d,
  ClintObjectType_image3d,
  ClintObjectType_program,
  ClintObjectType_kernel,
  ClintObjectType_event,
  ClintObjectType_sampler,
  ClintObjectType_device
} ClintObjType;

typedef enum ClintObjSharing {
  ClintObjectSharing_none,
  ClintObjectSharing_gl,
  ClintObjectSharing_d3d9,
  ClintObjectSharing_d3d10,
  ClintObjectSharing_d3d11
} ClintObjSharing;

#define CLINT_DEFINE_OBJ_FUNCS(type)                                \
typedef struct ClintObject_##type {                                 \
  CLINT_TREE_ELEMS(struct ClintObject_##type, cl_##type);           \
  char *stack;                                                      \
  cl_context context;                                               \
  ClintAtomicInt refCount;                                          \
  ClintAtomicInt threadCount;                                       \
} ClintObject_##type;                                               \
                                                                    \
ClintObject_##type *clint_lookup_##type(cl_##type v);               \
void clint_check_input_##type(cl_##type v);                         \
void clint_check_output_##type(cl_##type v, void *src, ClintObjType t ARGS); \
void clint_check_input_##type##s(cl_uint num, const cl_##type *v);  \
void clint_check_output_##type##s(cl_uint num, cl_##type *v, void *src, ClintObjType t ARGS); \
void clint_retain_##type(cl_##type v);                              \
void clint_release_##type(cl_##type v)                              \

#define ARGS
CLINT_DEFINE_OBJ_FUNCS(context);
CLINT_DEFINE_OBJ_FUNCS(command_queue);
#undef ARGS
#define ARGS , cl_mem_flags flags, ClintObjSharing sharing
CLINT_DEFINE_OBJ_FUNCS(mem);
#undef ARGS
#define ARGS
CLINT_DEFINE_OBJ_FUNCS(program);
CLINT_DEFINE_OBJ_FUNCS(kernel);
CLINT_DEFINE_OBJ_FUNCS(event);
CLINT_DEFINE_OBJ_FUNCS(sampler);
CLINT_DEFINE_OBJ_FUNCS(device_id);

void clint_acquire_shared_mem(cl_mem v, ClintObjSharing sharing);
void clint_acquire_shared_mems(cl_uint num, const cl_mem *v, ClintObjSharing sharing);
void clint_release_shared_mem(cl_mem v, ClintObjSharing sharing);
void clint_release_shared_mems(cl_uint num, const cl_mem *v, ClintObjSharing sharing);

/* After OpenCL 1.0, only clSetKernelArg is not thread safe. */
void clint_kernel_enter(cl_kernel kernel);
void clint_kernel_exit(cl_kernel kernel);

/* Log any possible leaks for context, or all leaks if NULL. */
void clint_log_leaks(cl_context context);
void clint_log_leaks_all(void);

#ifdef __cplusplus
}
#endif

#endif // _CLINT_OBJ_H_
