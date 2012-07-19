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
  /* Track all OpenCL resources. */
  CLINT_TRACK,
  /* Log all OpenCL calls. */
  CLINT_CHECK_REFS,
  /* Check for threading errors. */
  CLINT_CHECK_THREAD,
  /* Check for any concurrent calls, even if allowed by OpenCL. */
  CLINT_STRICT_THREAD,
  /* Enable full debugging. */
  CLINT_CHECK_ALL,
  /* Abort when an error is encountered. */
  CLINT_ABORT,
  /* Last item. */
  CLINT_MAX
} ClintConfig;

void clint_config_init(const ClintPathChar *path);
int clint_get_config(ClintConfig v);

#endif
