To build the library:

git clone git://git.code.sf.net/p/clintercept/code clintercept
cd clintercept
git clone git://gitorious.org/findopencl/findopencl FindOpenCL
mkdir xcode
cd xcode
cmake -G Xcode ..
xcodebuild -project CLIntercept.xcodeproj -target ALL_BUILD -configuration Debug build

To use the library:

Window:

Copy OpenCL.dll and CLInterceptConfig.txt next to the application.
Edit the options in CLInterceptConfig.txt, especially the log file location.

Mac OS X:

Override DYLD_INSERT_LIBRARIES to the dylib's path and set any options in the environment.
You may also specify option in CLInterceptConfig.txt.
Logging will use NSLog by default, but a log file can be specified.

DYLD_INSERT_LIBRARIES=/my/install/path/libCLIntercept.dylib CLINT_CHECK_ALL=1 ./clinfo

Why is this a dylib not a framework?  If we write a replacement OpenCL framework, we can't
load the real framework.  We use DYLD_INSERT_LIBRARIES to inject replacement functions,
and use interposing internally to avoid DYLD_FORCE_FLAT_NAMESPACE.

Linux:

LD_PRELOAD=/my/install/path/libOpenCL.so LD_LIBRARY_PATH=/my/install/path/ CLINT_CHECK_ALL=1 ./clinfo

To run the test:
DYLD_INSERT_LIBRARIES=/my/install/path/libCLIntercept.dylib CLINT_CHECK_ALL=1 CLINT_EMBEDDED=1 ./test_clint all

Yes, it does print errors.  That's intentional because these are illegal kernels or OpenCL calls.
It shouldn't crash.  Note that you must enable CLIntercept to run this test.  It checks for this,
because your OpenCL driver won't catch these mistakes and it may crash your display or worse.

Usage:

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
="protect"
Enforce read-only and write-only mappings, but without guard pages.  Write-only can only
be enforced on systems that support PROT_WRITE in mmap() without PROT_READ.
(Windows does not support write-only.)
="guard"
The intermediate buffer will be aligned to the end of a page and followed by a guard page.
This is the default.
="guard_before"
Add guard pages before and after the intermediate buffer.  A gap of up to a page will
be preset between the buffer and the last guard page.
="malloc"
Allocate using malloc() to avoid interfering with other memory debugging tools.

CLINT_CHECK_ACQUIRE
Detect errors when sharing OpenGL or D3D objects.  Note that we only intercept OpenCL calls,
so synchronization is not checked.  OpenCL 1.2 Extension Specification, section 9.6.6.

CLINT_CHECK_BOUNDS
** Currently unimplemented **
Modify kernel source to allow memory bounds checking.  This won't work with binaries.

CLINT_CHECK_ALL
Enable the following options:
CLINT_CHECK_THREAD, CLINT_CHECK_MAPPING, CLINT_CHECK_ACQUIRE, CLINT_CHECK_BOUNDS.

CLINT_EMBEDDED
Enforce the minimum embedded profile requirements.
CL_PLATFORM_PROFILE and CL_DEVICE_PROFILE are set to EMBEDDED_PROFILE.
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
