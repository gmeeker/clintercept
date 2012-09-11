To use the library:

Window:

Copy OpenCL.dll and ClintConfig.txt next to the application.
Edit the options in ClintConfig.txt, especially the log file location.

Mac OS X:

Override DYLD_INSERT_LIBRARIES to the dylib's path and set any options in the environment.
You may also specify option in ClintConfig.txt.
Logging will use NSLog by default, but a log file can be specified.

DYLD_INSERT_LIBRARIES=/my/install/path/libClint.dylib CLINT_CHECK_ALL=1 ./clinfo

Why is this a dylib not a framework?  If we write a replacement OpenCL framework, we can't
load the real framework.  We use DYLD_INSERT_LIBRARIES to inject replacement functions,
and use interposing internally to avoid DYLD_FORCE_FLAT_NAMESPACE.

Linux:

LD_PRELOAD=/my/install/path/libOpenCL.so LD_LIBRARY_PATH=/my/install/path/ CLINT_CHECK_ALL=1 ./clinfo

CLINT_CONFIG_FILE <file>
Read configuration settings from <file>.

CLINT_LOG_FILE <file>
Write to <file> instead of standard error or NSLog.  <file> can also be 1 or stdout,
or 2 or stderr.

CLINT_TRACE
Log all OpenCL calls.

CLINT_ERRORS
Log all OpenCL errors.

CLINT_ABORT
Call abort() when any error is detected.

CLINT_INFO
Log platform and device information during startup.

CLINT_PROFILE
Profile all kernel execution and log the results.

CLINT_PROFILE_ALL
Profile all expensive calls and log the results.  This will wait on events and affect
performance.

CLINT_TRACK
Track all OpenCL objects.  This can discover when a previously released object is used.
Most of these options (other than logging) will turn this on.

CLINT_ZOMBIES
Remember information about previously released objects.

CLINT_LEAKS
Report any leaked objects at exit or when the last context is released.

CLINT_STACK_LOGGING
Record the program's call stack during object allocation.

CLINT_CHECK_THREAD
Check for thread safety.  All calls are thread-safe except clSetKernelArg with the same
cl_kernel.  OpenCL 1.2 Specification, A.2.

CLINT_STRICT_THREAD
Check for OpenCL 1.0 thread safety.  Only calls that create, retain, and retain objects are
thread-safe.  OpenCL 1.0 Specification, A.2.

CLINT_CHECK_MAPPING
Allocate intermediate memory buffers when mapping images or buffers.  This can help
detect buffer overwrites and allow other bounds checking tools to detect errors.

CLINT_CHECK_MAPPING_BOUNDS
Allocate extra memory to look for overwrites.  This may interfere with other
bounds checking tools.

CLINT_CHECK_ACQUIRE
Detect errors when sharing OpenGL or D3D objects.  Note that we only intercept OpenCL calls,
so synchronization is not checked.  OpenCL 1.2 Extension Specification, section 9.6.6.

CLINT_CHECK_BOUNDS
Modify kernel source to allow memory bounds checking.  This won't work with binaries.

CLINT_CHECK_ALL
Enable the following options:
CLINT_CHECK_THREAD, CLINT_CHECK_MAPPING, CLINT_CHECK_ACQUIRE, CLINT_CHECK_BOUNDS.

CLINT_EMBEDDED
Enforce the minimum embedded profile requirements.
CL_PLATFORM_PROFILE is set to EMBEDDED_PROFILE.
64-bit integers are supported unless cles_khr_int64 is disabled.
3D images are not supported.
read_imagef and read_imageh don't support CL_FILTER_LINEAR for CL_FLOAT or CL_HALF_FLOAT.
__EMBEDDED_PROFILE__ is defined.
The compiler is still available.
No other changes are made.
OpenCL 1.2 Extension Specification, section 10.

CLINT_DISABLE_IMAGE
Remove CL_DEVICE_IMAGE_SUPPORT.
The following queries will return zero:
  CL_DEVICE_IMAGE_SUPPORT
  CL_DEVICE_MAX_READ_IMAGE_ARGS
  CL_DEVICE_MAX_WRITE_IMAGE_ARGS
  CL_DEVICE_IMAGE2D_MAX_WIDTH
  CL_DEVICE_IMAGE2D_MAX_HEIGHT
  CL_DEVICE_IMAGE3D_MAX_WIDTH
  CL_DEVICE_IMAGE3D_MAX_HEIGHT
  CL_DEVICE_IMAGE3D_MAX_DEPTH
  CL_DEVICE_IMAGE_MAX_BUFFER_SIZE
  CL_DEVICE_IMAGE_MAX_ARRAY_SIZE
  CL_DEVICE_MAX_SAMPLERS
The following functions will return CL_INVALID_OPERATION:
  clCreateImage
  clCreateImage2D
  clCreateImage3D
  clGetImageInfo
  clEnqueueReadImage
  clEnqueueWriteImage
  clEnqueueFillImage
  clEnqueueCopyImage
  clEnqueueCopyImageToBuffer
  clEnqueueCopyBufferToImage
  clEnqueueMapImage
  clCreateFromGLTexture
  clCreateFromGLTexture2D
  clCreateFromGLTexture3D
  clGetGLTextureInfo
  clCreateFromD3D10Texture2DKHR
  clCreateFromD3D10Texture3DKHR
  clCreateFromD3D11Texture2DKHR
  clCreateFromD3D11Texture3DKHR
clGetSupportedImageFormats is not disabled.

CLINT_DISABLE_EXTENSION <ext>
Remove <ext> from the extension list.  Most behavior is not enforced.

CLINT_FORCE_DEVICE <dev>
Only <dev> will appear to the application.  OpenGL or D3D sharing, however, may still
select a different device.  <dev> may also be one of the CL_DEVICE_TYPE values.
The CL_DEVICE_TYPE_ prefix is optional.
