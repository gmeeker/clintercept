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

#include "clint_obj.h"
#include "clint_config.h"
#include "clint_data.h"
#include "clint_mem.h"
#include "clint_stack.h"

#include <string.h>

#if defined(WIN32)
#include <malloc.h>
#endif

#ifndef CL_MAP_WRITE_INVALIDATE_REGION
#define CL_MAP_WRITE_INVALIDATE_REGION (1 << 2)
#endif

static size_t clint_sizeof_channel_type(cl_channel_type data_type);
static size_t clint_sizeof_image_format(const cl_image_format *image_format);

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
    return NULL;                                                    \
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
    memset(obj, 0, sizeof(ClintObject_##type));                     \
    COPYARGS;                                                       \
    if (!VALID_DYN_OBJ(obj)) {                                      \
      ClintObject_##type *obj2 = NULL;                              \
      clint_purge_##type(v);                                        \
      CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                     \
      obj2 = clint_tree_find_ClintObject_##type(g_clint_objects_##type, v); \
      CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                   \
      if (obj2 != NULL) {                                           \
        free(obj);                                                  \
        return;                                                     \
      }                                                             \
    }                                                               \
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
  if (VALID_DYN_OBJ(obj)) {                                         \
    CLINT_ATOMIC_ADD(1, obj->refCount);                             \
  }                                                                 \
}                                                                   \
                                                                    \
void clint_release_##type(cl_##type v)                              \
{                                                                   \
  ClintObject_##type *obj = clint_lookup_##type(v);                 \
  if (VALID_DYN_OBJ(obj)) {                                         \
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
void clint_purge_##type(cl_##type v)                                \
{                                                                   \
  if (clint_get_config(CLINT_TRACK) &&                              \
      clint_get_config(CLINT_ZOMBIES)) {                            \
    ClintObject_##type *obj = NULL;                                 \
    CLINT_SPINLOCK_LOCK(g_clint_lock_##type);                       \
    obj = clint_tree_find_ClintObject_##type(g_clint_objects_##type, v); \
    if (VALID_DYN_OBJ(obj)) {                                       \
      if (obj->refCount == 0) {                                     \
        clint_tree_erase_ClintObject_##type(&g_clint_objects_##type, obj); \
        if (obj->stack) {                                           \
          free(obj->stack);                                         \
        }                                                           \
        free(obj);                                                  \
      }                                                             \
    }                                                               \
    CLINT_SPINLOCK_UNLOCK(g_clint_lock_##type);                     \
  }                                                                 \
}                                                                   \
                                                                    \
static void clint_log_leaks_##type(ClintObject_##type *tree, cl_context context) \
{                                                                   \
  ClintObject_##type *iter;                                         \
  int zombies = clint_get_config(CLINT_ZOMBIES);                    \
  for (iter = clint_tree_first_ClintObject_##type(tree);            \
       iter;                                                        \
       iter = clint_tree_next_ClintObject_##type(iter)) {           \
    if (VALID_DYN_OBJ(iter) &&                                      \
        (context == NULL || context == iter->context) &&            \
        (!zombies || iter->refCount > 0)) {                         \
      clint_log("Possibly leaked cl_" #type ": %p\n", iter->_key);  \
      if (iter->stack != NULL)                                      \
        clint_log("Created at:\n%s\n", iter->stack);                \
    }                                                               \
  }                                                                 \
}                                                                   \

#define ARGS
#define ARGNAMES
#define COPYARGS
#define VALID_DYN_OBJ(O) ((O) != NULL)
CLINT_IMPL_OBJ_FUNCS(context);
CLINT_IMPL_OBJ_FUNCS(command_queue);
#undef ARGS
#undef ARGNAMES
#undef COPYARGS
#define ARGS , cl_mem_flags flags, ClintObjSharing sharing, const cl_image_format *image_format
#define ARGNAMES , flags, sharing, image_format
#define COPYARGS obj->flags = flags; obj->sharing = sharing; obj->pixelSize = clint_sizeof_image_format(image_format)
CLINT_IMPL_OBJ_FUNCS(mem);
#undef ARGS
#undef ARGNAMES
#undef COPYARGS
#define ARGS
#define ARGNAMES
#define COPYARGS
CLINT_IMPL_OBJ_FUNCS(program);
CLINT_IMPL_OBJ_FUNCS(kernel);
CLINT_IMPL_OBJ_FUNCS(event);
CLINT_IMPL_OBJ_FUNCS(sampler);
#undef ARGS
#undef ARGNAMES
#undef COPYARGS
#undef VALID_DYN_OBJ
#define ARGS , cl_bool subdevice
#define ARGNAMES , subdevice
#define COPYARGS obj->subdevice = subdevice
#define VALID_DYN_OBJ(O) ((O) != NULL && (O)->subdevice)
CLINT_IMPL_OBJ_FUNCS(device_id);
#undef ARGS
#undef ARGNAMES
#undef COPYARGS
#undef VALID_DYN_OBJ

static size_t clint_sizeof_channel_type(cl_channel_type data_type)
{
  switch (data_type) {
  case CL_SNORM_INT8:
  case CL_UNORM_INT8:
  case CL_SIGNED_INT8:
  case CL_UNSIGNED_INT8:
    return 1;
  case CL_SNORM_INT16:
  case CL_UNORM_INT16:
  case CL_SIGNED_INT16:
  case CL_UNSIGNED_INT16:
    return 2;
  case CL_SIGNED_INT32:
  case CL_UNSIGNED_INT32:
    return 4;
  case CL_HALF_FLOAT:
    return 2;
  case CL_FLOAT:
    return 4;
#ifdef CL_SFIXED14_APPLE
  case CL_SFIXED14_APPLE:
    return 2;
#endif
#ifdef CL_BIASED_HALF_APPLE
  case CL_BIASED_HALF_APPLE:
    return 2;
#endif
  default:
    return 0;
  }
}

static size_t clint_sizeof_image_format(const cl_image_format *image_format)
{
  if (image_format == NULL)
    return 0;
  switch (image_format->image_channel_order) {
  case CL_R:
  case CL_A:
  case CL_INTENSITY:
  case CL_LUMINANCE:
    return clint_sizeof_channel_type(image_format->image_channel_data_type);
  case CL_RG:
  case CL_RA:
  case CL_Rx:
    return 2 * clint_sizeof_channel_type(image_format->image_channel_data_type);
  case CL_RGx:
    return 3 * clint_sizeof_channel_type(image_format->image_channel_data_type);
  case CL_RGB:
  case CL_RGBx:
    switch (image_format->image_channel_data_type) {
    case CL_UNORM_SHORT_565:
      return 2;
    case CL_UNORM_SHORT_555:
      return 2;
    case CL_UNORM_INT_101010:
      return 4;
    default:
      return 0;
    }
  case CL_RGBA:
  case CL_ARGB:
  case CL_BGRA:
    return 4 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#ifdef CL_ABGR_APPLE
  case CL_ABGR_APPLE:
    return 4 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#endif
#ifdef CL_1RGB_APPLE
  case CL_1RGB_APPLE:
    return 3 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#endif
#ifdef CL_BGR1_APPLE
  case CL_BGR1_APPLE:
    return 3 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#endif
#ifdef CL_YCbYCr_APPLE
  case CL_YCbYCr_APPLE:
    return 2 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#endif
#ifdef CL_CbYCrY_APPLE
  case CL_CbYCrY_APPLE:
    return 2 * clint_sizeof_channel_type(image_format->image_channel_data_type);
#endif
  default:
    return 0;
  }
}

void clint_set_image_format(cl_mem v, const cl_image_format *image_format)
{
  if (clint_get_config(CLINT_TRACK)) {
    if (image_format != NULL) {
      ClintObject_mem *obj = clint_lookup_mem(v);
      if (obj != NULL) {
        obj->pixelSize = clint_sizeof_image_format(image_format);
      }
    }
  }
}

void *clint_retain_map(cl_mem v, cl_map_flags map_flags, void *ptr, size_t size)
{
  if (clint_get_config(CLINT_TRACK)) {
    ClintObject_mem *obj = clint_lookup_mem(v);
    if (obj != NULL) {
      ClintAtomicInt count = CLINT_ATOMIC_ADD(1, obj->mapCount);
      if (count == 1 &&
          clint_get_config(CLINT_CHECK_MAPPING)) {
        if ((obj->flags & CL_MEM_USE_HOST_PTR) != 0) {
          /* The application may expect the pointer to be the same. */
          return ptr;
        }
        obj->mapPtr = ptr;
        obj->mapSize = size;
        obj->mapFlags = map_flags;
        if (clint_get_config_string(CLINT_CHECK_MAPPING) == NULL ||
            clint_cmp_config_string(CLINT_CHECK_MAPPING, "malloc") == 0) {
          /* Allocate with SIMD alignment. */
          const size_t align = 32;
#if defined(WIN32)
          obj->mapCopy.addr = _mm_malloc(obj->mapSize, align);
#else
          if (posix_memalign(&obj->mapCopy.addr, align, obj->mapSize) != 0)
            obj->mapCopy.addr = NULL;
#endif
          if (obj->mapCopy.addr == NULL) {
            return ptr;
          }
          if ((obj->mapFlags & CL_MAP_WRITE_INVALIDATE_REGION) == 0) {
            memcpy(obj->mapCopy.addr, obj->mapPtr, obj->mapSize);
          }
        } else {
          unsigned int flags = 0;
          if (clint_cmp_config_string(CLINT_CHECK_MAPPING, "protect") == 0) {
          } else if (clint_cmp_config_string(CLINT_CHECK_MAPPING, "guard_before") == 0) {
            flags |= ClintMemProtection_Guard_Before | ClintMemProtection_Guard_After;
          } else {
            flags |= ClintMemProtection_Guard_After;
          }
          if (clint_mem_alloc(&obj->mapCopy, obj->mapSize,
                              flags | ClintMemProtection_Read | ClintMemProtection_Write)) {
            return ptr;
          }
          if ((obj->mapFlags & CL_MAP_WRITE_INVALIDATE_REGION) == 0) {
            memcpy(obj->mapCopy.addr, obj->mapPtr, obj->mapSize);
          }
          if ((obj->mapFlags & CL_MAP_READ) != 0) {
            flags |= ClintMemProtection_Read;
          }
          if ((obj->mapFlags & CL_MAP_WRITE) != 0 ||
              (obj->mapFlags & CL_MAP_WRITE_INVALIDATE_REGION) != 0) {
            flags |= ClintMemProtection_Write;
          }
          if (clint_mem_protect(&obj->mapCopy, flags)) {
            clint_mem_free(&obj->mapCopy);
            return ptr;
          }
        }
        ptr = obj->mapCopy.addr;
      }
    }
  }
  return ptr;
}

void *clint_retain_map_image(cl_mem v, cl_map_flags map_flags, void *ptr,
                             const size_t *region,
                             size_t *image_row_pitch,
                             size_t *image_slice_pitch)
{
  if (clint_get_config(CLINT_TRACK)) {
    ClintObject_mem *obj = clint_lookup_mem(v);
    if (obj != NULL) {
      size_t size = 0;
      if (image_slice_pitch) {
        size += (region[2]-1) * *image_slice_pitch;
      }
      size += (region[1]-1) * *image_row_pitch;
      size += region[0] * obj->pixelSize;
      return clint_retain_map(v, map_flags, ptr, size);
    }
  }
  return ptr;
}

void clint_release_map(cl_mem v)
{
  if (clint_get_config(CLINT_TRACK)) {
    ClintObject_mem *obj = clint_lookup_mem(v);
    if (obj != NULL) {
      ClintAtomicInt count = CLINT_ATOMIC_SUB(1, obj->mapCount);
      if (count == 0 &&
          clint_get_config(CLINT_CHECK_MAPPING)) {
        if (obj->mapCopy.addr != NULL &&
            ((obj->mapFlags & CL_MAP_WRITE) != 0 ||
             (obj->mapFlags & CL_MAP_WRITE_INVALIDATE_REGION) != 0)) {
          if (obj->mapCopy.addr_real == NULL ||
              clint_mem_protect(&obj->mapCopy, ClintMemProtection_Read) == 0) {
            memcpy(obj->mapPtr, obj->mapCopy.addr, obj->mapSize);
          }
        }
        if (obj->mapCopy.addr_real == NULL) {
#if defined(WIN32)
          _mm_free(obj->mapCopy.addr);
#else
          free(obj->mapCopy.addr);
#endif
        } else {
          clint_mem_free(&obj->mapCopy);
        }
      }
    }
  }
}

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

struct DeviceType {
  const char *name;
  cl_device_type type;
};

static struct DeviceType s_device_types[] = {
  { "CL_DEVICE_TYPE_CPU", CL_DEVICE_TYPE_CPU },
  { "CL_DEVICE_TYPE_GPU", CL_DEVICE_TYPE_GPU },
  { "CL_DEVICE_TYPE_ACCELERATOR", CL_DEVICE_TYPE_ACCELERATOR },
  { "CL_DEVICE_TYPE_DEFAULT", CL_DEVICE_TYPE_DEFAULT },
  { "CL_DEVICE_TYPE_ALL", CL_DEVICE_TYPE_ALL },
  { NULL, CL_DEVICE_TYPE_ALL }
};

cl_device_type clint_modify_device_type(cl_device_type device_type)
{
  if (clint_get_config(CLINT_FORCE_DEVICE) && clint_get_config_string(CLINT_FORCE_DEVICE)[0]) {
    size_t i;
    for (i = 0; s_device_types[i].name != NULL; i++) {
      /* Compare with and without CL_DEVICE_TYPE_ */
      if (clint_cmp_config_string(CLINT_FORCE_DEVICE, s_device_types[i].name) == 0 ||
          clint_cmp_config_string(CLINT_FORCE_DEVICE, s_device_types[i].name + 15) == 0) {
        return s_device_types[i].type;
      }
    }
    clint_log("WARNING: CLINT_FORCE_DEVICE=%s wasn't matched in clCreateContextFromType.  Try a CL_DEVICE_TYPE_* constant.\n", clint_get_config_string(CLINT_FORCE_DEVICE));
  }
  return device_type;
}

const cl_device_id *clint_modify_context_devices(const cl_context_properties *properties, cl_uint *num_devices, const cl_device_id *devices)
{
  size_t i;
  cl_device_id *new_devices;
  cl_platform_id platform = NULL;
  cl_uint num, dev_idx;
  cl_int err;

  if (clint_get_config(CLINT_FORCE_DEVICE) && clint_get_config_string(CLINT_FORCE_DEVICE)[0]) {
    if (properties == NULL) {
      clint_log("WARNING: CLINT_FORCE_DEVICE won't override NULL properties.\n");
      return devices;
    }
    for (i = 0; properties[i] != 0; i += 2) {
      if (properties[i] == CL_CONTEXT_PLATFORM) {
        platform = (cl_platform_id)properties[i+1];
      }
    }
    if (platform == NULL) {
      clint_log("WARNING: CLINT_FORCE_DEVICE won't override without a platform argument.\n");
      return NULL;
    }
    for (i = 0; s_device_types[i].name != NULL; i++) {
      /* Compare with and without CL_DEVICE_TYPE_ */
      if (clint_cmp_config_string(CLINT_FORCE_DEVICE, s_device_types[i].name) == 0 ||
          clint_cmp_config_string(CLINT_FORCE_DEVICE, s_device_types[i].name + 15) == 0) {
        err = clGetDeviceIDs(platform, s_device_types[i].type, 0, NULL, &num);
        if (err == CL_SUCCESS) {
          devices = new_devices = clint_autopool_malloc(num * sizeof(cl_device_id));
          err = clGetDeviceIDs(platform, s_device_types[i].type, num, new_devices, NULL);
          *num_devices = num;
        }
        return devices;
      }
    }
    err = clGetDeviceIDs(platform, s_device_types[i].type, 0, NULL, &num);
    if (err == CL_SUCCESS) {
      new_devices = clint_autopool_malloc(num * sizeof(cl_device_id));
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num, new_devices, NULL);
      if (err == CL_SUCCESS) {
        for (dev_idx = 0; dev_idx < num; dev_idx++) {
          size_t size;
          err = clGetDeviceInfo(devices[dev_idx], CL_DEVICE_NAME, 0, NULL, &size);
          if (err == CL_SUCCESS) {
            char *name = clint_autopool_malloc(size);
            err = clGetDeviceInfo(devices[dev_idx], CL_DEVICE_NAME, size, name, NULL);
            if (err == CL_SUCCESS && clint_cmp_config_string(CLINT_FORCE_DEVICE, name) == 0) {
              devices = new_devices = clint_autopool_malloc(sizeof(cl_device_id));
              *new_devices = devices[dev_idx];
              *num_devices = 1;
              return devices;
            }
          }
        }
      }
    }
    clint_log("WARNING: CLINT_FORCE_DEVICE=%s wasn't matched.\n", clint_get_config_string(CLINT_FORCE_DEVICE));
  }
  return devices;
}

const char *clint_modify_build_options(const char *options)
{
  if (clint_get_config(CLINT_EMBEDDED)) {
    return clint_string_cat("-D __EMBEDDED_PROFILE__ ", options);
  }
  return options;
}

static const char *clint_embedded_prefix =
  "static bool clint_is_float(int v) { return v == CLK_HALF_FLOAT || v == CLK_FLOAT; }\n"
  "#undef read_imagef\n"
  "#undef read_imagei\n"
  "#undef read_imageui\n"
  "#undef read_imageh\n"
//  "#define read_imagef(a,b,c) ((clint_is_float(get_image_channel_data_type((a))) && ((b) & CLK_FILTER_LINEAR) != 0) ? (printf(\"Illegal use of CLK_FILTER_LINEAR\\n\"), (float4)(0.f)) : read_imagef((a),(b),(c)))\n"
  "#define read_imagef(a,b,c) ((clint_is_float(get_image_channel_data_type((a))) && ((b) & CLK_FILTER_LINEAR) != 0) ? (float4)(0.f) : read_imagef((a),(b),(c)))\n"
  "#define read_imagei\n"
  "#define read_imageui\n"
  "#define read_imageh\n"
  ;

const char *clint_modify_program_source(const char *source)
{
  if (clint_get_config(CLINT_EMBEDDED)) {
    return clint_string_cat(clint_embedded_prefix, source);
  }
  return source;
}

const char **clint_modify_program_sources(cl_uint count, const char **strings, const size_t **lengths)
{
  if (count > 0) {
    const char *oldstr = strings[0];
    const char *newstr;
    if (*lengths != NULL && (*lengths)[0] > 0) {
      char *s = (char*)clint_autopool_malloc((*lengths)[0] + 1);
      memcpy(s, strings[0], (*lengths)[0]);
      s[(*lengths)[0]] = 0;
      oldstr = s;
    }
    newstr = clint_modify_program_source(oldstr);
    if (newstr != oldstr) {
      void *newstringsbuf = clint_autopool_malloc((size_t)count * sizeof(const char*));
      const char **newstrings = (const char **)newstringsbuf;
      memcpy(newstringsbuf, strings, (size_t)count * sizeof(const char*));
      newstrings[0] = newstr;
      if (*lengths != NULL) {
        size_t *newlengths = (size_t*)clint_autopool_malloc((size_t)count * sizeof(size_t));
        memcpy(newlengths, *lengths, (size_t)count * sizeof(size_t));
        newlengths[0] = 0;
      }
      strings = newstrings;
    }
  }
  return strings;
}
