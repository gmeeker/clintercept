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

#ifndef _CLINT_RES_H_
#define _CLINT_RES_H_

#include "clint_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLINT_DEFINE_RES_FUNCS(type)                                \
void clint_check_input_##type(cl_##type v);                         \
void clint_check_output_##type(cl_##type v);                        \
void clint_check_input_##type##s(cl_uint num, const cl_##type *v);  \
void clint_check_output_##type##s(cl_uint num, cl_##type *v);       \
void clint_retain_##type(cl_##type v);                              \
void clint_release_##type(cl_##type v)                              \

CLINT_DEFINE_RES_FUNCS(context);
CLINT_DEFINE_RES_FUNCS(command_queue);
CLINT_DEFINE_RES_FUNCS(mem);
CLINT_DEFINE_RES_FUNCS(program);
CLINT_DEFINE_RES_FUNCS(kernel);
CLINT_DEFINE_RES_FUNCS(event);
CLINT_DEFINE_RES_FUNCS(sampler);

/* After OpenCL 1.0, only clSetKernelArg is not thread safe. */
void clint_kernel_enter(cl_kernel kernel);
void clint_kernel_exit(cl_kernel kernel);

#ifdef __cplusplus
}
#endif

#endif // _CLINT_RES_H_
