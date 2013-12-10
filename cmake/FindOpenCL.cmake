# - Try to find OpenCL
# This module tries to find an OpenCL implementation on your system.
# This is a combination of:
# FindOpenCL.cmake (git://gitorious.org/findopencl/findopencl) and
# OpenCV's OpenCVDetectOpenCL.cmake (which has better support for Win32,
# but doesn't search for the full OS X framework path).
#
# To set manually the paths, define these environment variables:
# OPENCL_ROOT_DIR - Top path (e.g. OPENCL_ROOT_DIR=/usr/lib64/nvidia)
#
# Once done this will define
#  OPENCL_FOUND        - system has OpenCL
#  OPENCL_INCLUDE_DIRS  - the OpenCL include directory
#  OPENCL_LIBRARIES    - link these to use OpenCL
#
# WIN32 should work, but is untested

FIND_PACKAGE(PackageHandleStandardArgs)

if(CMAKE_CL_64)
  set(MSVC64 1)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  if(WIN32)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
              OUTPUT_VARIABLE CMAKE_OPENCV_GCC_TARGET_MACHINE
              OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(CMAKE_OPENCV_GCC_TARGET_MACHINE MATCHES "amd64|x86_64|AMD64")
      set(MINGW64 1)
    endif()
  endif()
endif()

if(MSVC64 OR MINGW64)
  set(X86_64 1)
elseif(MINGW OR (MSVC AND NOT CMAKE_CROSSCOMPILING))
  set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
  set(X86_64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
  set(X86 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm.*|ARM.*")
  set(ARM 1)
endif()

if(APPLE)
  set(OPENCL_FOUND YES)
  find_library(OPENCL_LIBRARY OpenCL DOC "OpenCL lib for OSX")
  find_path(OPENCL_INCLUDE_DIR OpenCL/cl.h DOC "Include for OpenCL on OSX")
  mark_as_advanced(OPENCL_INCLUDE_DIR OPENCL_LIBRARY)
else(APPLE)
  if (NOT OPENCL_FOUND)
    find_path(OPENCL_ROOT_DIR
              NAMES OpenCL/cl.h CL/cl.h include/CL/cl.h include/nvidia-current/CL/cl.h
              PATHS ENV OCLROOT ENV AMDAPPSDKROOT ENV CUDA_PATH ENV INTELOCLSDKROOT
              DOC "OpenCL root directory"
              NO_DEFAULT_PATH)

    find_path(OPENCL_INCLUDE_DIR
              NAMES OpenCL/cl.h CL/cl.h
              HINTS ${OPENCL_ROOT_DIR}
              PATH_SUFFIXES include include/nvidia-current
              DOC "OpenCL include directory"
              NO_DEFAULT_PATH)

    if (X86_64)
      set(OPENCL_POSSIBLE_LIB_SUFFIXES lib/Win64 lib/x86_64 lib/x64)
    elseif (X86)
      set(OPENCL_POSSIBLE_LIB_SUFFIXES lib/Win32 lib/x86)
    endif()

    find_library(OPENCL_LIBRARY
              NAMES OpenCL
              HINTS ${OPENCL_ROOT_DIR}
              PATH_SUFFIXES ${OPENCL_POSSIBLE_LIB_SUFFIXES}
              DOC "OpenCL library"
              NO_DEFAULT_PATH)

    mark_as_advanced(OPENCL_INCLUDE_DIR OPENCL_LIBRARY)
  endif()
endif(APPLE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENCL DEFAULT_MSG OPENCL_LIBRARY OPENCL_INCLUDE_DIR)

if(OPENCL_FOUND)
  set(HAVE_OPENCL 1)
  set(OPENCL_INCLUDE_DIRS ${OPENCL_INCLUDE_DIR})
  set(OPENCL_LIBRARIES    ${OPENCL_LIBRARY})
endif()
