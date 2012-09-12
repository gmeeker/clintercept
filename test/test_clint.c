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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

struct Program {
  cl_program program;
  cl_kernel kernel;
};

static const char *null_program_text =
  "kernel void kmain(const float s) {}\n";

static const char *bounds_program_text =
  "kernel void kmain(global float4 *dst)\n"
  "{\n"
  "}\n";

cl_int createProgram(cl_context context, struct Program *program, const char *text)
{
  cl_device_id *devices;
  size_t i, num_devices;
  cl_int err;

  memset(program, 0, sizeof(struct Program));
  program->program = clCreateProgramWithSource(context, 1, &text, NULL, &err);
  if (err != CL_SUCCESS)
    return err;
  err = clBuildProgram(program->program, 0, NULL, "-cl-mad-enable", NULL, NULL);
  if (err != CL_SUCCESS) {
    cl_int err0 = err;
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &num_devices);
    if (err)
      return err;
    num_devices /= sizeof(cl_device_id);
    devices = (cl_device_id*)malloc(num_devices * sizeof(cl_device_id));
    if (devices == NULL)
      return CL_OUT_OF_HOST_MEMORY;
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, num_devices * sizeof(cl_device_id), devices, NULL);
    if (err)
      return err;
    for (i = 0; i < num_devices; i++) {
      cl_build_status build_status;
      err = clGetProgramBuildInfo(program->program, devices[i], CL_PROGRAM_BUILD_STATUS,
                                  sizeof(build_status), &build_status, NULL);
      if (err)
        return err;
      if (build_status == CL_BUILD_ERROR) {
        char *str;
        size_t logSize;
        err = clGetProgramBuildInfo(program->program, devices[i], CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
        if (err)
          return err;
        str = (char*)malloc(logSize);
        err = clGetProgramBuildInfo(program->program, devices[i], CL_PROGRAM_BUILD_LOG, logSize, str, NULL);
        if (err)
          return err;
        fprintf(stderr, "ERROR: OpenCL error during compiling:\n%s\n", str);
      }
    }
    return err0;
  }
  program->kernel = clCreateKernel(program->program, "kmain", &err);
  return err;
}

void destroyProgram(struct Program *program)
{
  if (program->kernel != NULL) {
    clReleaseKernel(program->kernel);
    program->kernel = NULL;
  }
  if (program->program != NULL) {
    clReleaseProgram(program->program);
    program->program = NULL;
  }
}

#ifdef WIN32
static unsigned int WINAPI do1Thread(void *data)
#else
static void* do1Thread(void *data)
#endif
{
  cl_kernel kernel = (cl_kernel)data;
  cl_float arg = 0.f;
  int i;

  /* Set this big enough to get thread conflicts in clSetKernelArg */
  for (i = 0; i < 1000; i++) {
    /* Ignore errors because this isn't legal and may crash... */
    (void)clSetKernelArg(kernel, 0, sizeof(arg), &arg);
  }
#ifdef WIN32
  _endthreadex(0);
  return 0;
#else
  return NULL;
#endif
}

static void testThread(cl_context context, cl_command_queue queue)
{
  struct Program program;
  cl_int err = createProgram(context, &program, null_program_text);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Unable to compile OpenCL program: %d.\n", err);
    return;
  }
#ifdef WIN32
  HANDLE threads[2];
  unsigned int id;
  DWORD thread_return;
  threads[0] = (HANDLE)_beginthreadex(NULL, 0, do1Thread, program.kernel, 0, &id);
  threads[1] = (HANDLE)_beginthreadex(NULL, 0, do1Thread, program.kernel, 0, &id);
  WaitForSingleObject(threads[0], INFINITE);
  GetExitCodeThread(threads[0], &thread_return);
  WaitForSingleObject(threads[1], INFINITE);
  GetExitCodeThread(threads[1], &thread_return);
#else
  pthread_t threads[2];
  if (pthread_create(&threads[0], NULL, do1Thread, program.kernel) >= 0 &&
      pthread_create(&threads[1], NULL, do1Thread, program.kernel) >= 0) {
    void *thread_return;
    pthread_join(threads[0], &thread_return);
    pthread_join(threads[1], &thread_return);
  }
#endif
  destroyProgram(&program);
}

static void testMapping(cl_context context, cl_command_queue queue)
{
  const size_t size = 1024;
  cl_int err;
  cl_mem mem;
  void *ptr;
  void *ptr2;
  cl_event event;
  mem = clCreateBuffer(context, CL_MEM_READ_WRITE, size, NULL, NULL);
  if (mem == NULL) {
    return;
  }
  ptr = clEnqueueMapBuffer(queue, mem, CL_TRUE, CL_MAP_READ, 0, size, 0, NULL, NULL, &err);
  fprintf(stderr, "Reading past buffer...\n");
  (void)((uint8_t*)ptr)[size];
  fprintf(stderr, "Writing read-only buffer...\n");
  ((uint8_t*)ptr)[0] = 0;
  if (err == CL_SUCCESS) {
    clEnqueueUnmapMemObject(queue, mem, ptr, 0, NULL, &event);
    err = clWaitForEvents(1, &event);
    if (err != CL_SUCCESS) {
      return;
    }
  }

  ptr2 = clEnqueueMapBuffer(queue, mem, CL_TRUE, CL_MAP_WRITE, 0, size, 0, NULL, NULL, &err);
  fprintf(stderr, "Writing past buffer...\n");
  ((uint8_t*)ptr2)[size] = 0;
  fprintf(stderr, "Reading write-only buffer...\n");
  (void)((uint8_t*)ptr2)[0];
  if (err == CL_SUCCESS) {
    clEnqueueUnmapMemObject(queue, mem, ptr2, 0, NULL, &event);
    err = clWaitForEvents(1, &event);
    if (err != CL_SUCCESS) {
      return;
    }
  }
}

static void testBounds(cl_context context, cl_command_queue queue)
{
}

int main(int argc, const char *argv[])
{
  const char *test;
  cl_int err;
  cl_context context;
  cl_command_queue queue;
  cl_context_properties properties[3];
  cl_platform_id *platforms;
  cl_device_id *devices;
  char *ext;
  cl_uint num_platforms;
  size_t num_devices;
  size_t ext_size;
  int i;

  test = NULL;
  if (argc >= 2) {
    test = argv[1];
  }
  if (test == NULL) {
    fprintf(stderr, "Usage: %s all|leaks|thread|mapping|bounds\n", argv[0]);
    return 1;
  }
  if (strcmp(test, "all") == 0) {
    test = "";
  }

  err = clGetPlatformIDs(0, NULL, &num_platforms);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL platforms: %d.\n", err);
    exit(EXIT_FAILURE);
  }
  platforms = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id));
  if (platforms == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  err = clGetPlatformIDs(num_platforms, platforms, NULL);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL platforms: %d.\n", err);
    exit(EXIT_FAILURE);
  }

  err = clGetPlatformInfo(platforms[0], CL_PLATFORM_EXTENSIONS, 0, NULL, &ext_size);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL extensions: %d.\n", err);
    exit(EXIT_FAILURE);
  }
  ext = (char*)malloc(ext_size);
  err = clGetPlatformInfo(platforms[0], CL_PLATFORM_EXTENSIONS, ext_size, ext, NULL);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL extensions: %d.\n", err);
    exit(EXIT_FAILURE);
  }

  if (strstr(ext, "cl_CLINT_debugging") == NULL) {
    fprintf(stderr, "ERROR: Test suite is not being run through Clint.\n");
    exit(EXIT_FAILURE);
  }

  i = 0;
  properties[i++] = (cl_context_properties)CL_CONTEXT_PLATFORM;
  properties[i++] = (cl_context_properties)platforms[0];
  properties[i++] = (cl_context_properties)0;

  context = clCreateContextFromType(properties,
                                    CL_DEVICE_TYPE_GPU,
                                    NULL,
                                    NULL,
                                    &err);
  if (err) {
    context = clCreateContextFromType(properties,
                                      CL_DEVICE_TYPE_ACCELERATOR,
                                      NULL,
                                      NULL,
                                      &err);
  }
  if (err) {
    context = clCreateContextFromType(properties,
                                      CL_DEVICE_TYPE_CPU,
                                      NULL,
                                      NULL,
                                      &err);
  }
  if (err) {
    fprintf(stderr, "Unable to create OpenCL context: %d.\n", err);
    exit(EXIT_FAILURE);
  }

  err = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &num_devices);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL devices: %d.\n", err);
    exit(EXIT_FAILURE);
  }
  num_devices /= sizeof(cl_device_id);
  devices = (cl_device_id*)malloc(num_devices * sizeof(cl_device_id));
  if (devices == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  err = clGetContextInfo(context, CL_CONTEXT_DEVICES, num_devices * sizeof(cl_device_id), devices, NULL);
  if (err) {
    fprintf(stderr, "Unable to query OpenCL devices: %d.\n", err);
    exit(EXIT_FAILURE);
  }

  queue = clCreateCommandQueue(context, devices[0], 0, &err);
  if (err) {
    fprintf(stderr, "Unable to create OpenCL queue: %d.\n", err);
    exit(EXIT_FAILURE);
  }

  if (test[0] == 0 || strcmp(test, "thread") == 0) {
    testThread(context, queue);
  }
  if (test[0] == 0 || strcmp(test, "mapping") == 0) {
    testMapping(context, queue);
  }
  if (test[0] == 0 || strcmp(test, "bounds") == 0) {
    testBounds(context, queue);
  }
  if (!(test[0] == 0 || strcmp(test, "leaks") == 0)) {
    err = clReleaseCommandQueue(queue);
    if (err) {
      fprintf(stderr, "Error releasing OpenCL queue: %d.\n", err);
      exit(EXIT_FAILURE);
    }
    err = clReleaseContext(context);
    if (err) {
      fprintf(stderr, "Error releasing OpenCL context: %d.\n", err);
      exit(EXIT_FAILURE);
    }
  } else {
    cl_mem mem = clCreateBuffer(context, CL_MEM_READ_WRITE, 1024, NULL, NULL);
    cl_sampler sampler = clCreateSampler(context, CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_NEAREST, NULL);
  }
}
