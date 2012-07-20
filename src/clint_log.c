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

void clint_log(const char *fmt, ...)
{
  char *buf;
  size_t size;
  va_list ap;
  char tmp[8];

  va_start(ap, fmt);
  if (g_clint_log_fp != NULL) {
    vfprintf(g_clint_log_fp, fmt, ap);
  } else {
    size = (size_t)vsprintf_s(tmp, sizeof(tmp), fmt, ap) + 1;
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
    g_clint_log_fp = fopen(filename, "w");
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

typedef long long clint_log_int_t;

static void clint_log_device_raw(cl_device_id device, cl_device_info param, const char *name, const char *value)
{
  clint_log("\tdevice[%p]: %s = %s\n", device, name, value);
}

static cl_bool clint_get_device_int(cl_device_id device, cl_device_info param, const char *name, clint_log_int_t *v)
{
  cl_int err;
  size_t size;

  if ((err = clGetDeviceInfo(device, param, sizeof(*v), v, &size)) == CL_SUCCESS) {
    if (size > sizeof(*v)) {
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

static void clint_log_device_int(cl_device_id device, cl_device_info param, const char *name, cl_bool asHex)
{
  cl_int err;
  size_t size;
  clint_log_int_t v;

  if ((err = clGetDeviceInfo(device, param, sizeof(v), &v, &size)) == CL_SUCCESS) {
    if (size > sizeof(v)) {
      clint_log("\tdevice[%p]: %s unexpected size: %lu\n", device, name, (unsigned long)size);
    }
    if (asHex) {
      clint_log("\tdevice[%p]: %s = 0x%llx\n", device, name, v);
    } else {
      clint_log("\tdevice[%p]: %s = %lld\n", device, name, v);
    }
    if (param == CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) {
      size_t items[3];
      items[0] = items[1] = items[2] = 0;
      if ((err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*(size_t)v, items, &size)) == CL_SUCCESS) {
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
          clint_log("\tdevice[%p]: %s[%ld] = %s\n", device, name, (unsigned long)i, clint_string_device_partition_property(buf[i]));
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
  clint_log_int_t value_int;

  clint_log_device_string(device, DEVICE_ARGS(NAME));
  clint_log_device_string(device, DEVICE_ARGS(VENDOR));
  clint_log_device_string(device, DEVICE_ARGS(PROFILE));
  clint_log_device_string(device, DEVICE_ARGS(VERSION));
  clint_log_device_string(device, DEVICE_ARGS(EXTENSIONS));
  clint_log_device_string(device, DEVICE_ARGS(OPENCL_C_VERSION));
  clint_log_device_string(device, LOG_ARGS(CL_DRIVER_VERSION));

  if (clint_get_device_int(device, DEVICE_ARGS(TYPE), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(TYPE),
                         clint_string_device_type((cl_device_type)value_int));
  }
  clint_log_device_int(device, DEVICE_ARGS(PLATFORM), CL_TRUE);
  if (clint_get_device_int(device, DEVICE_ARGS(EXECUTION_CAPABILITIES), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(EXECUTION_CAPABILITIES),
                         clint_string_device_exec_capabilities((cl_device_exec_capabilities)value_int));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_TYPE), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_TYPE),
                         clint_string_device_mem_cache_type((cl_device_mem_cache_type)value_int));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(LOCAL_MEM_TYPE), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(LOCAL_MEM_TYPE),
                         clint_string_device_local_mem_type((cl_device_local_mem_type)value_int));
  }

  if (clint_get_device_int(device, DEVICE_ARGS(SINGLE_FP_CONFIG), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(SINGLE_FP_CONFIG),
                         clint_string_device_fp_config((cl_device_fp_config)value_int));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(DOUBLE_FP_CONFIG), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(DOUBLE_FP_CONFIG),
                         clint_string_device_fp_config((cl_device_fp_config)value_int));
  }
  if (clint_get_device_int(device, DEVICE_ARGS(HALF_FP_CONFIG), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(HALF_FP_CONFIG),
                         clint_string_device_fp_config((cl_device_fp_config)value_int));
  }
  clint_log_device_int(device, DEVICE_ARGS(QUEUE_PROPERTIES), CL_TRUE);

  clint_log_device_int(device, DEVICE_ARGS(VENDOR_ID), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_COMPUTE_UNITS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WORK_ITEM_DIMENSIONS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WORK_GROUP_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_CHAR), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_SHORT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_INT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_LONG), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_FLOAT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_DOUBLE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CLOCK_FREQUENCY), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ADDRESS_BITS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_MEM_ALLOC_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_SUPPORT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_READ_IMAGE_ARGS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_WRITE_IMAGE_ARGS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE2D_MAX_WIDTH), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE2D_MAX_HEIGHT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_WIDTH), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_HEIGHT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE3D_MAX_DEPTH), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_SAMPLERS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_PARAMETER_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MEM_BASE_ADDR_ALIGN), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MIN_DATA_TYPE_ALIGN_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHELINE_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_CACHE_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(GLOBAL_MEM_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CONSTANT_BUFFER_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(MAX_CONSTANT_ARGS), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(LOCAL_MEM_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ERROR_CORRECTION_SUPPORT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PROFILING_TIMER_RESOLUTION), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(ENDIAN_LITTLE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(AVAILABLE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(COMPILER_AVAILABLE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_VECTOR_WIDTH_HALF), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(HOST_UNIFIED_MEMORY), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_CHAR), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_SHORT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_INT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_LONG), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_FLOAT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_DOUBLE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(NATIVE_VECTOR_WIDTH_HALF), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(LINKER_AVAILABLE), CL_FALSE);
  clint_log_device_string(device, DEVICE_ARGS(BUILT_IN_KERNELS));
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_MAX_BUFFER_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(IMAGE_MAX_ARRAY_SIZE), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PARENT_DEVICE), CL_TRUE);
  clint_log_device_int(device, DEVICE_ARGS(PARTITION_MAX_SUB_DEVICES), CL_FALSE);
  clint_log_device_partition(device, DEVICE_ARGS(PARTITION_PROPERTIES));
  if (clint_get_device_int(device, DEVICE_ARGS(PARTITION_AFFINITY_DOMAIN), &value_int)) {
    clint_log_device_raw(device, DEVICE_ARGS(PARTITION_AFFINITY_DOMAIN),
                         clint_string_device_affinity_domain((cl_device_affinity_domain)value_int));
  }
  clint_log_device_partition(device, DEVICE_ARGS(PARTITION_TYPE));
  clint_log_device_int(device, DEVICE_ARGS(REFERENCE_COUNT), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PREFERRED_INTEROP_USER_SYNC), CL_FALSE);
  clint_log_device_int(device, DEVICE_ARGS(PRINTF_BUFFER_SIZE), CL_FALSE);

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
