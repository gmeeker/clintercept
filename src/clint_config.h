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

#ifndef _CLINT_CONFIG_H_
#define _CLINT_CONFIG_H_

#include "clint.h"

typedef enum ClintConfig {
  /* Is anything enabled? */
  CLINT_ENABLED = 0,
  /* Path for config file */
  CLINT_CONFIG_FILE,
  /* Path for log file */
  CLINT_LOG_FILE,
  /* Log all OpenCL calls. */
  CLINT_TRACE,
  /* Log all OpenCL errors. */
  CLINT_ERRORS,
  /* Abort when an error is encountered. */
  CLINT_ABORT,
  /* Print OpenCL device info at startup. */
  CLINT_INFO,
  /* Profile kernel execution. */
  CLINT_PROFILE,
  /* Profile all calls. */
  CLINT_PROFILE_ALL,
  /* Track all OpenCL resources. */
  CLINT_TRACK,
  /* Remember deallocated resources. */
  CLINT_ZOMBIES,
  /* Report any leaked resources. */
  CLINT_LEAKS,
  /* Log stack during resource allocation. */
  CLINT_STACK_LOGGING,
  /* Check for threading errors.  Currently only clSetKernelArg. */
  CLINT_CHECK_THREAD,
  /* Check for OpenCL 1.0 thread safety. */
  CLINT_STRICT_THREAD,
  /* Allocate intermediate memory buffers when mapping images or buffers. */
  CLINT_CHECK_MAPPING,
  /* Detect errors when sharing OpenGL or D3D objects. */
  CLINT_CHECK_ACQUIRE,
  /* Modify kernel source to allow memory bounds checking. */
  CLINT_CHECK_BOUNDS,
  /* Enable full checking. */
  CLINT_CHECK_ALL,
  /* Enforce the minimum embedded profile requirements. */
  CLINT_EMBEDDED,
  /* Remove CL_DEVICE_IMAGE_SUPPORT and cause clCreateImage2D etc. to fail. */
  CLINT_DISABLE_IMAGE,
  /* Remove ext from the extension list.  Most behavior is not enforced. */
  CLINT_DISABLE_EXTENSION,
  /* Only <dev> will appear to the application. */
  CLINT_FORCE_DEVICE,
  /* Last item. */
  CLINT_MAX
} ClintConfig;

void clint_config_init(const ClintPathChar *path);
int clint_get_config(ClintConfig which);
const char *clint_get_config_string(ClintConfig which);
void clint_set_config(ClintConfig which, int v);
int clint_cmp_config_string(ClintConfig which, const char *s);
const char *clint_config_describe(ClintConfig which);

#endif
