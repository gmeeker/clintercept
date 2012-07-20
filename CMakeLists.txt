# Clint OpenCL utilities
cmake_minimum_required (VERSION 2.8)
project (Clint)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/FindOpenCL/")
find_package(OpenCL REQUIRED)

include_directories(${OPENCL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CLINT_LIBNAME "OpenCL")
set(CMAKE_OSX_ARCHITECTURES i386;x86_64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)

if (APPLE)
  set(OPENCL_INCLUDE_SEARCH "${OPENCL_INCLUDE_DIRS}/Headers/")
else()
  set(OPENCL_INCLUDE_SEARCH ${OPENCL_INCLUDE_DIRS})
endif()

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c
#  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensource.py /System/Library/Frameworks/OpenCL.framework/Headers/{cl.h,cl_ext.h,cl_gl.h,cl_gl_ext.h} -o ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensource.py ${OPENCL_INCLUDE_SEARCH}/{cl.h,cl_ext.h,cl_gl.h,cl_gl_ext.h} -o ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensource.py
  )

add_executable (clinfo ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.c src/clinfo.c src/clint_config.c src/clint_data.c src/clint_log.c src/clint_thread.c ${OPENCL_LIBRARIES})

if (APPLE)
  find_library(CARBON_LIBRARY Carbon)
  find_library(FOUNDATION_LIBRARY Foundation)
  mark_as_advanced(CARBON_LIBRARY
                   FOUNDATION_LIBRARY)
  set(EXTRA_LIBS ${CARBON_LIBRARY} ${FOUNDATION_LIBRARY})
else()
  set(EXTRA_LIBS )
endif()

add_library (${CLINT_LIBNAME} ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_funcs.c ${CMAKE_CURRENT_BINARY_DIR}/clint_opencl_types.c src/clint.c src/clint_config.c src/clint_data.c src/clint_log.c src/clint_thread.c ${EXTRA_LIBS})
if (APPLE)
  set_target_properties(${CLINT_LIBNAME} PROPERTIES FRAMEWORK TRUE)
  set_target_properties(${CLINT_LIBNAME} PROPERTIES FRAMEWORK_VERSION A)
         
  # setup Info.plist
  #set_target_properties(${CLINT_LIBNAME} PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.sfml-dev.${CLINT_LIBNAME})
  #set_target_properties(${CLINT_LIBNAME} PROPERTIES MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  #set_target_properties(${CLINT_LIBNAME} PROPERTIES MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
 
#  install(TARGETS ${CLINT_LIBNAME}
#          FRAMEWORK DESTINATION "/Library/Frameworks" COMPONENT bin)
 
else()
#  install(TARGETS ${CLINT_LIBNAME}
#          RUNTIME DESTINATION bin COMPONENT bin
#          LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT bin 
#          ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT devel)
endif()
