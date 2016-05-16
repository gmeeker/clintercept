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

#include "clint_log.h"
#include "clint_config.h"
#include "clint_opencl_types.h"

#include <stdio.h>

#ifdef __APPLE__

#include <OpenCL/cl_gl.h>
#include <OpenCL/cl_gl_ext.h>
#include <OpenCL/cl_ext.h>

#else

#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_ext.h>

#endif

#ifndef CL_VERSION_1_2
typedef intptr_t            cl_device_partition_property;
typedef cl_bitfield         cl_device_affinity_domain;
#define clint_string_device_affinity_domain clint_string_token
#define clint_string_device_partition_property clint_string_token
#endif

#ifndef CL_DEVICE_DOUBLE_FP_CONFIG
#define CL_DEVICE_DOUBLE_FP_CONFIG                  0x1032
#endif
#ifndef CL_DEVICE_HALF_FP_CONFIG
#define CL_DEVICE_HALF_FP_CONFIG                    0x1033
#endif
#ifndef CL_DEVICE_LINKER_AVAILABLE
#define CL_DEVICE_LINKER_AVAILABLE                  0x103E
#endif
#ifndef CL_DEVICE_BUILT_IN_KERNELS
#define CL_DEVICE_BUILT_IN_KERNELS                  0x103F
#endif
#ifndef CL_DEVICE_IMAGE_MAX_BUFFER_SIZE
#define CL_DEVICE_IMAGE_MAX_BUFFER_SIZE             0x1040
#endif
#ifndef CL_DEVICE_IMAGE_MAX_ARRAY_SIZE
#define CL_DEVICE_IMAGE_MAX_ARRAY_SIZE              0x1041
#endif
#ifndef CL_DEVICE_PARENT_DEVICE
#define CL_DEVICE_PARENT_DEVICE                     0x1042
#endif
#ifndef CL_DEVICE_PARTITION_MAX_SUB_DEVICES
#define CL_DEVICE_PARTITION_MAX_SUB_DEVICES         0x1043
#endif
#ifndef CL_DEVICE_PARTITION_PROPERTIES
#define CL_DEVICE_PARTITION_PROPERTIES              0x1044
#endif
#ifndef CL_DEVICE_PARTITION_AFFINITY_DOMAIN
#define CL_DEVICE_PARTITION_AFFINITY_DOMAIN         0x1045
#endif
#ifndef CL_DEVICE_PARTITION_TYPE
#define CL_DEVICE_PARTITION_TYPE                    0x1046
#endif
#ifndef CL_DEVICE_REFERENCE_COUNT
#define CL_DEVICE_REFERENCE_COUNT                   0x1047
#endif
#ifndef CL_DEVICE_PREFERRED_INTEROP_USER_SYNC
#define CL_DEVICE_PREFERRED_INTEROP_USER_SYNC       0x1048
#endif
#ifndef CL_DEVICE_PRINTF_BUFFER_SIZE
#define CL_DEVICE_PRINTF_BUFFER_SIZE                0x1049
#endif

#define ERR(a) case a: return #a;
#define LOG_ARGS(a) a, #a
#define ARG_WITH_SIZE(a) &a, sizeof(a)
#define DEVICE_ARGS(a) CL_DEVICE_##a, "CL_DEVICE_" #a
#define PLATFORM_ARGS(a) CL_PLATFORM_##a, "CL_PLATFORM_" #a

static FILE *g_clint_log_fp;
static int g_clint_log_fp_close;

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>

#ifdef __cplusplus
extern "C"
#endif
void NSLogv(CFStringRef, va_list);

void clint_log(const char *fmt, ...)
{
  CFStringRef str;
  va_list ap;

  va_start(ap, fmt);
  if (g_clint_log_fp != NULL) {
    vfprintf(g_clint_log_fp, fmt, ap);
  } else {
    str = CFStringCreateWithCString(kCFAllocatorDefault, fmt, kCFStringEncodingASCII);
    if (str != NULL) {
      NSLogv(str, ap);
      CFRelease(str);
    }
  }
  va_end(ap);
}

#elif defined(WIN32)

#include <windows.h>

void clint_log(const char *fmt, ...)
{
  char *buf;
  size_t size;
  va_list ap;

  va_start(ap, fmt);
  if (g_clint_log_fp != NULL) {
    vfprintf(g_clint_log_fp, fmt, ap);
  } else {
    size = (size_t)_vscprintf(fmt, ap) + 1;
    buf = (char*)malloc(size);
    vsprintf_s(buf, size, fmt, ap);
    OutputDebugStringA(buf);
    free(buf);
  }
  va_end(ap);
}

#else

void clint_log(const char *fmt, ...)
{
  va_list ap;
  FILE *fp = g_clint_log_fp;
  if (fp == NULL)
    fp = stderr;

  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
}

#endif

void clint_log_init(const char *filename)
{
  clint_log_shutdown();
  if (filename != NULL) {
#if defined(WIN32)
    if (fopen_s(&g_clint_log_fp, filename, "w") != 0) {
      g_clint_log_fp = NULL;
    }
    /* so we can tell which log file came from which process */
    char processName[MAX_PATH];
    GetProcessImageFileNameA(GetCurrentProcess(), processName,
                             sizeof(processName));
    fprintf_s(g_clint_log_fp, "Process %ld, %s\n",
              (long)clint_get_process_id(), processName);
#else
    g_clint_log_fp = fopen(filename, "w");
    /* so we can tell which log file came from which process */
    fprintf(g_clint_log_fp, "Process %ld, %s\n",
            (long)clint_get_process_id(), getprogname());
#endif
    g_clint_log_fp_close = 1;
  }
}

void clint_log_init_fp(FILE *fp)
{
  clint_log_shutdown();
  g_clint_log_fp = fp;
  g_clint_log_fp_close = 0;
}

void clint_log_shutdown()
{
  if (g_clint_log_fp != NULL && g_clint_log_fp_close) {
    fclose(g_clint_log_fp);
    g_clint_log_fp = NULL;
    g_clint_log_fp_close = 0;
  }
}

void clint_log_abort()
{
  if (clint_get_config(CLINT_ABORT)) {
    abort();
  }
}

void clint_log_describe()
{
  int i;

  for (i = 0; i < CLINT_MAX; i++) {
    if (i != CLINT_ENABLED) {
      if (clint_get_config(i)) {
        clint_log(clint_config_describe(i));
      }
    }
  }
}

static void clint_log_device_raw(cl_device_id device, cl_device_info param, const char *name, const char *value)
{
  clint_log("\tdevice[%p]: %s = %s\n", device, name, value);
}

static cl_bool clint_get_device_int(cl_device_id device, cl_device_info param, const char *name, void *v, size_t size_v)
{
  cl_int err;
  size_t size;

  if ((err = clGetDeviceInfo(device, param, size_v, v, &size)) == CL_SUCCESS) {
    if (size > size_v) {
      clint_log("\tdevice[%p]: %s unexpected size: %lu\n", device, name, (unsigned long)size);
    }
    return CL_TRUE;
  } else {
    clint_log("\tdevice[%p]: %s: %s\n", device, name, clint_string_error(err));
    return CL_FALSE;
  }
}

static void clint_log_device_string(cl_device_id device, cl_device_info param, const char *name)
{
  cl_int err;
  size_t size;

  if ((err = clGetDeviceInfo(device, param, 0, NULL, &size)) == CL_SUCCESS) {
    void *buf = malloc(size);
    if (buf != NULL) {
      if ((err = clGetDeviceInfo(device, param, size, buf, NULL)) == CL_SUCCESS) {
        clint_log("\tdevice[%p]: %s = %s\n", device, name, (const char*)buf);
      }
      free(buf);
    }
  } else {
    clint_log("\tdevice[%p]: %s: %s\n", device, name, clint_string_error(err));
  }
}

static void clint_log_device_version(cl_device_id device, cl_device_info param, const char *name,
                                     int *major, int *minor)
{
  cl_int err;
  size_t size;

  *major = 1;
  *minor = 0;
  if ((err = clGetDeviceInfo(device, param, 0, NULL, &size)) == CL_SUCCESS) {
    void *buf = malloc(size);
    if (buf != NULL) {
      if ((err = clGetDeviceInfo(device, param, size, buf, NULL)) == CL_SUCCESS) {
        int scan_count;
#if defined(WIN32)
        scan_count = sscanf_s((const char*)buf, " OpenCL %d.%d", major, minor);
#else
        scan_count = sscanf((const char*)buf, " OpenCL %d.%d", major, minor);
#endif
        if (scan_count != 2) {
          *major = 1;
          *minor = 0;
        }
        clint_log("\tdevice[%p]: %s = %s\n", device, name, (const char*)buf);
      }
      free(buf);
    }
  } else {
    clint_log("\tdevice[%p]: %s: %s\n", device, name, clint_string_error(err));
  }
}

static void clint_log_device_int(cl_device_id device, cl_device_info param, const char *name, size_t size_v, cl_bool asHex)
{
  cl_int err;
  size_t size;
  union {
    char c;
    short s;
    int i;
    long l;
    long long ll;
    cl_uint clui;
  } v;

  if ((err = clGetDeviceInfo(device, param, size_v, &v, &size)) == CL_SUCCESS) {
    if (size > size_v) {
      clint_log("\tdevice[%p]: %s unexpected size: %lu\n", device, name, (unsigned long)size);
    }
    if (size_v == sizeof(char)) {
      if (asHex) {
        clint_log("\tdevice[%p]: %s = 0x%x\n", device, name, (int)v.c);
      } else {
        clint_log("\tdevice[%p]: %s = %d\n", device, name, (int)v.c);
      }
    } else if (size_v == sizeof(short)) {
      if (asHex) {
        clint_log("\tdevice[%p]: %s = 0x%x\n", device, name, (int)v.s);
      } else {
        clint_log("\tdevice[%p]: %s = %d\n", device, name, (int)v.s);
      }
    } else if (size_v == sizeof(int)) {
      if (asHex) {
        clint_log("\tdevice[%p]: %s = 0x%x\n", device, name, v.i);
      } else {
        clint_log("\tdevice[%p]: %s = %d\n", device, name, v.i);
      }
    } else if (size_v == sizeof(long)) {
      if (asHex) {
        clint_log("\tdevice[%p]: %s = 0x%lx\n", device, name, v.l);
      } else {
        clint_log("\tdevice[%p]: %s = %ld\n", device, name, v.l);
      }
    } else if (size_v == sizeof(long long)) {
      if (asHex) {
        clint_log("\tdevice[%p]: %s = 0x%llx\n", device, name, v.ll);
      } else {
        clint_log("\tdevice[%p]: %s = %lld\n", device, name, v.ll);
      }
    }
    if (param == CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) {
      size_t items[3];
      items[0] = items[1] = items[2] = 0;
      if ((err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*(size_t)v.clui, items, &size)) == CL_SUCCESS) {
        clint_log("\tdevice[%p]: %s: (%lu, %lu, %lu)\n",
                  device, "CL_MAX_WORK_ITEM_SIZES", (unsigned long)items[0], (unsigned long)items[1], (unsigned long)items[2]);
      }
    }
  } else {
    clint_log("\tdevice[%p]: %s: %s\n", device, name, clint_string_error(err));
  }
}

static void clint_log_device_partition(cl_device_id device, cl_device_info param, const char *name)
{
  cl_int err;
  size_t i, size;

  if ((err = clGetDeviceInfo(device, param, 0, NULL, &size)) == CL_SUCCESS) {
    cl_device_partition_property *buf = (cl_device_partition_property*)malloc(size);
    if (buf != NULL) {
      if ((err = clGetDeviceInfo(device, param, size, buf, NULL)) == CL_SUCCESS) {
        size /= sizeof(cl_device_partition_property);
        for (i = 0; i < size; i++) {
          clint_log("\tdevice[%p]: %s[%ld] = %s\n", device, name, (unsigned long)i,
                    (buf[i] ? clint_string_device_partition_property(buf[i]) : "0"));
        }
      }
      free(buf);
    }
  } else {
    clint_log("\tdevice[%p]: %s: %s\n", device, name, clint_string_error(err));
  }
}

static void clint_log_platform_string(cl_platform_id platform, cl_platform_info param, const char *name)
{
  cl_int err;
  size_t size;

  if ((err = clGetPlatformInfo(platform, param, 0, NULL, &size)) == CL_SUCCESS) {
    void *buf = malloc(size);
    if (buf != NULL) {
      if ((err = clGetPlatformInfo(platform, param, size, buf, NULL)) == CL_SUCCESS) {
        clint_log("platform[%p]: %s = %s\n", platform, name, (const char*)buf);
      }
      free(buf);
    }
  } else {
    clint_log("platform[%p]: %s: %s\n", platform, name, clint_string_error(err));
  }
}

void clint_log_device_formats(cl_platform_id platform, cl_device_id device)
{
   cl_context_properties properties[16];
   cl_image_format *formatList;
   cl_context context;
   cl_int err;
   cl_uint numFormats;
   int i, j;

   i = 0;
   properties[i++] = (cl_context_properties)CL_CONTEXT_PLATFORM;
   properties[i++] = (cl_context_properties)platform;
   properties[i++] = (cl_context_properties)0;

   context = clCreateContext(properties, 1, &device, NULL, NULL, &err);
   if (err == CL_SUCCESS) {
     cl_mem_flags flags;
     cl_mem_object_type type;
     for (i = 0; i < 3; i++) {
       switch (i) {
       default:
       case 0:
         flags = CL_MEM_READ_ONLY;
         break;
       case 1:
         flags = CL_MEM_WRITE_ONLY;
         break;
       case 2:
         flags = CL_MEM_READ_WRITE;
         break;
       }
       for (j = 0; j < 2; j++) {
         switch (j) {
         default:
         case 0:
           type = CL_MEM_OBJECT_IMAGE2D;
           break;
         case 1:
           type = CL_MEM_OBJECT_IMAGE3D;
           break;
         }
         clint_log("\tdevice[%p]: %s %s.\n",
                   device,
                   clint_string_mem_flags(flags),
                   clint_string_mem_object_type(type));
         if ((err = clGetSupportedImageFormats(context, flags, type, 0, NULL, &numFormats)) == CL_SUCCESS) {
           clint_log("\tdevice[%p]: Found %d format(s).\n", device, numFormats);
           if (numFormats > 0) {
             formatList = (cl_image_format*)malloc(numFormats * sizeof(cl_image_format));
             if ((err = clGetSupportedImageFormats(context, flags, type, numFormats, formatList, NULL)) == CL_SUCCESS) {
               cl_uint k;
               for (k = 0; k < numFormats; k++) {
                 clint_log("\tdevice[%p]: %s\t%s\n",
                           device,
                           clint_string_channel_order(formatList[k].image_channel_order),
                           clint_string_channel_type(formatList[k].image_channel_data_type));
               }
             } else {
               clint_log("\tdevice[%p]: Unable to enumerate the formats: %s\n",
                         device, clint_string_error(err));
               free(formatList);
             }
             free(formatList);
           }
         } else {
           clint_log("\tdevice[%p]: Unable to query the number of formats: %s\n", device, clint_string_error(err));
         }
       }
     }
     clReleaseContext(context);
   } else {
     clint_log("\tdevice[%p]: Unable to create context: %s\n", device, clint_string_error(err));
   }
}

void clint_log_device(cl_platform_id platform, cl_device_id device)
{
  cl_device_type device_type;
  cl_device_exec_capabilities exec_capabilities;
  cl_device_mem_cache_type mem_cache_type;
  cl_device_local_mem_type local_mem_type;
  cl_device_fp_config fp_config;
  cl_command_queue_properties queue_properties;
  cl_device_affinity_domain affinity_domain;

  int major=1, minor=0;

  clint_log_device_string(device, DEVICE_ARGS(NAME));
  clint_log_device_string(device, DEVICE_ARGS(VENDOR));
  clint_log_device_string(device, DEVICE_ARGS(PROFILE));
  clint_log_device_version(device, DEVICE_ARGS(VERSION), &major, &minor);
  clint_log_device_string(device, DEVICE_ARGS(EXTENSIONS));
  clint_log_device_string(device, DEVICE_ARGS(OPENCL_C_VERSION));
  clint_log_device_string(device, LOG_ARGS(CL_DRIVER_VERSION));

  if (clint_get_device_int(device, DEVICE_ARGS(TYPE),
                           ARG_WITH_SIZE(device_type))) {
    clint_log_device_raw(device, DEVICE_ARGS(TYPE),
                         clint_string_device_type(device_type));
  }
  clint_log_device_int(device, DEVICE_ARGS(PLATFORM), sizeof(cl_platform_id), CL_TRUE);
  if (clint_get_device_int(device, DEVICE_ARGS(EXECUTION_CAPABILITIES),
                           ARG_WITH_SIZE(exec_capabilities))) {
    clint_log_device_raw(device, DEVICE_ARGS(EXECUTION_CAPABILITIES),
                         clint_string_device_exec_capabilities(exec_capabilities));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_TYPE),
                           ARG_WITH_SIZE(mem_cache_type))) {
    clint_log_device_raw(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_TYPE),
                         clint_string_device_mem_cache_type(mem_cache_type));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(LOCAL_MEM_TYPE),
                           ARG_WITH_SIZE(local_mem_type))) {
    clint_log_device_raw(device, DEVICE_ARGS(LOCAL_MEM_TYPE),
                         clint_string_device_local_mem_type(local_mem_type));
  }

  if (clint_get_device_int(device, DEVICE_ARGS(SINGLE_FP_CONFIG),
                           ARG_WITH_SIZE(fp_config))) {
    clint_log_device_raw(device, DEVICE_ARGS(SINGLE_FP_CONFIG),
                         clint_string_device_fp_config(fp_config));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(DOUBLE_FP_CONFIG),
                           ARG_WITH_SIZE(fp_config))) {
    clint_log_device_raw(device, DEVICE_ARGS(DOUBLE_FP_CONFIG),
                         clint_string_device_fp_config(fp_config));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(HALF_FP_CONFIG),
                           ARG_WITH_SIZE(fp_config))) {
    clint_log_device_raw(device, DEVICE_ARGS(HALF_FP_CONFIG),
                         clint_string_device_fp_config(fp_config));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(QUEUE_PROPERTIES),
                           ARG_WITH_SIZE(queue_properties))) {
    clint_log_device_raw(device, DEVICE_ARGS(QUEUE_PROPERTIES),
                         clint_string_command_queue_properties(queue_properties));
  }

  clint_log_device_int(device, DEVICE_ARGS(VENDOR_ID), sizeof(cl_uint), CL_TRUE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_COMPUTE_UNITS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WORK_ITEM_DIMENSIONS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WORK_GROUP_SIZE), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_CHAR), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_SHORT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_INT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_LONG), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_FLOAT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_DOUBLE), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CLOCK_FREQUENCY), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ADDRESS_BITS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_MEM_ALLOC_SIZE), sizeof(cl_ulong), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_SUPPORT), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_READ_IMAGE_ARGS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WRITE_IMAGE_ARGS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE2D_MAX_WIDTH), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE2D_MAX_HEIGHT), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_WIDTH), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_HEIGHT), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_DEPTH), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_SAMPLERS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_PARAMETER_SIZE), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MEM_BASE_ADDR_ALIGN), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MIN_DATA_TYPE_ALIGN_SIZE), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHELINE_SIZE), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_SIZE), sizeof(cl_ulong), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_SIZE), sizeof(cl_ulong), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CONSTANT_BUFFER_SIZE), sizeof(cl_ulong), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CONSTANT_ARGS), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(LOCAL_MEM_SIZE), sizeof(cl_ulong), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ERROR_CORRECTION_SUPPORT), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PROFILING_TIMER_RESOLUTION), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ENDIAN_LITTLE), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(AVAILABLE), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(COMPILER_AVAILABLE), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_HALF), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(HOST_UNIFIED_MEMORY), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_CHAR), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_SHORT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_INT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_LONG), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_FLOAT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_DOUBLE), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_HALF), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(LINKER_AVAILABLE), sizeof(cl_bool), CL_FALSE);
  /* AMD CPU driver can crash from this call. */
  if (major >= 2 || (major == 1 && minor >= 2))
    clint_log_device_string(device, DEVICE_ARGS(BUILT_IN_KERNELS));
  else
    clint_log("\tdevice[%p]: %s: %s\n",
              device, "CL_BUILT_IN_KERNELS", clint_string_error(CL_INVALID_VALUE));
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_MAX_BUFFER_SIZE), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_MAX_ARRAY_SIZE), sizeof(size_t), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PARENT_DEVICE), sizeof(cl_device_id), CL_TRUE);
  clint_log_device_int(device, DEVICE_ARGS(PARTITION_MAX_SUB_DEVICES), sizeof(cl_uint), CL_FALSE);
  clint_log_device_partition(device, DEVICE_ARGS(PARTITION_PROPERTIES));
  if (clint_get_device_int(device, DEVICE_ARGS(PARTITION_AFFINITY_DOMAIN),
                           ARG_WITH_SIZE(affinity_domain))) {
    clint_log_device_raw(device, DEVICE_ARGS(PARTITION_AFFINITY_DOMAIN),
                         clint_string_device_affinity_domain(affinity_domain));
  }
  clint_log_device_partition(device, DEVICE_ARGS(PARTITION_TYPE));
  clint_log_device_int(device, DEVICE_ARGS(REFERENCE_COUNT), sizeof(cl_uint), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_INTEROP_USER_SYNC), sizeof(cl_bool), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PRINTF_BUFFER_SIZE), sizeof(size_t), CL_FALSE);

  clint_log_device_formats(platform, device);
}

void clint_log_platform(cl_platform_id platform)
{
  cl_int err;
  cl_uint count;
  cl_device_id *devices;

  if ((err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &count)) == CL_SUCCESS) {
    clint_log_platform_string(platform, PLATFORM_ARGS(NAME));
    clint_log_platform_string(platform, PLATFORM_ARGS(VENDOR));
    clint_log_platform_string(platform, PLATFORM_ARGS(PROFILE));
    clint_log_platform_string(platform, PLATFORM_ARGS(VERSION));
    clint_log_platform_string(platform, PLATFORM_ARGS(EXTENSIONS));

    clint_log("Found %u devices(s).\n", (unsigned int)count);
    devices = (cl_device_id*)malloc(sizeof(cl_device_id) * count);
    if (devices != NULL) {
      if ((err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, count, devices, NULL)) == CL_SUCCESS) {
        cl_uint i;
        for (i = 0; i < count; i++) {
          clint_log_device(platform, devices[i]);
        }
      }
      free(devices);
    }
  }
  if (err != CL_SUCCESS) {
    clint_log("clint_log_platform() failed: %s\n", clint_string_error(err));
  }
}

void clint_log_platforms()
{
  cl_int err;
  cl_uint count;
  cl_platform_id *platforms;

  if ((err = clGetPlatformIDs(0, NULL, &count)) == CL_SUCCESS) {
    clint_log("Found %u platform(s).\n", (unsigned int)count);
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * count);
    if (platforms != NULL) {
      if ((err = clGetPlatformIDs(count, platforms, NULL)) == CL_SUCCESS) {
        cl_uint i;
        for (i = 0; i < count; i++) {
          clint_log_platform(platforms[i]);
        }
      }
      free(platforms);
    }
  }
  if (err != CL_SUCCESS) {
    clint_log("clint_log_platforms() failed: %s\n", clint_string_error(err));
  }
}
