# CLIntercept OpenCL utilities
cmake_minimum_required (VERSION 2.8)
project (CLIntercept)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(OpenCL REQUIRED)
find_package(Threads REQUIRED)

include_directories(${OPENCL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (WIN32)
  set(CLINT_LIBNAME "OpenCL")
else()
  set(CLINT_LIBNAME "CLIntercept")
endif()
set(CMAKE_OSX_ARCHITECTURES x86_64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

set(CLINT_OPENCL_HEADERS "cl.h;cl_ext.h" CACHE STRING "Headers to scan for OpenCL functions.")
set(CLINT_USE_OPENGL ON CACHE BOOL "Support OpenCL/OpenGL sharing functions.")
set(CLINT_OPENGL_HEADERS "cl_gl.h;cl_gl_ext.h" CACHE STRING "Headers to scan for OpenCL/OpenGL functions.")
if (WIN32)
  set(CLINT_USE_D3D ON CACHE BOOL "Support OpenCL/Direct3D sharing functions.")
  set(CLINT_D3D_HEADERS "cl_dx9_media_sharing.h;cl_d3d10.h;cl_d3d11.h" CACHE STRING "Headers to scan for OpenCL/Direct3D functions.")
endif()

list(APPEND CLINT_SCAN_HEADERS ${CLINT_OPENCL_HEADERS})
if (${CLINT_USE_OPENGL})
  list(APPEND CLINT_SCAN_HEADERS ${CLINT_OPENGL_HEADERS})
endif()
if (${CLINT_USE_D3D})
  list(APPEND CLINT_SCAN_HEADERS ${CLINT_D3D_HEADERS})
endif()

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.c
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.h
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensource.py -i ${OPENCL_INCLUDE_DIRS} -o ${CMAKE_CURRENT_BINARY_DIR} ${CLINT_SCAN_HEADERS}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensource.py
  )

if (APPLE)
#  find_library(OPENCL_LIBRARY OpenCL)
#  find_library(FOUNDATION_LIBRARY Foundation)
#  mark_as_advanced(OPENCL_LIBRARY
#                   FOUNDATION_LIBRARY)
#  set(EXTRA_LIBS ${OPENCL_LIBRARY} ${FOUNDATION_LIBRARY})
  list(APPEND EXTRA_CLINT_LINK_FLAGS "-framework OpenCL" "-framework Foundation")
  list(APPEND EXTRA_LINK_FLAGS "-framework OpenCL" "-framework Foundation")
else()
  set(EXTRA_LIBS )
endif()

add_executable (clinfo ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.c src/clinfo.c src/clint_config.c src/clint_data.c src/clint_log.c src/clint_thread.c)
target_link_libraries(clinfo ${EXTRA_LINK_FLAGS} ${OPENCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_library (${CLINT_LIBNAME} SHARED ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.c src/clint.c src/clint_config.c src/clint_data.c src/clint_log.c src/clint_mem.c src/clint_obj.c src/clint_stack.c src/clint_thread.c src/clint_tree.c ${EXTRA_LIBS})
target_link_libraries(${CLINT_LIBNAME} ${EXTRA_CLINT_LINK_FLAGS} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
#  install(TARGETS ${CLINT_LIBNAME}
#          RUNTIME DESTINATION bin COMPONENT bin
#          LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT bin 
#          ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT devel)

add_executable (test_tree test/test_tree.c src/clint_tree.c)
add_executable (test_clint test/test_clint.c)
target_link_libraries(test_clint ${EXTRA_LINK_FLAGS} ${OPENCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
