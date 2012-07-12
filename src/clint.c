#include <stdlib.h>
#include <stdarg.h>

#ifdef __APPLE_CC__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#ifdef __APPLE_CC__

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>

static void* clint_opencl_load(void)
{
	FSRef frameworksRef;
	CFURLRef frameworksURL, url;
	CFBundleRef framework = NULL;
	if (FSFindFolder(kOnAppropriateDisk, kFrameworksFolderType,
					 kDontCreateFolder, &frameworksRef) != (OSErr)noErr) {
        return NULL;
	}
	frameworksURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &frameworksRef);
	if (frameworksURL == NULL)
		return NULL;
	url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorSystemDefault, frameworksURL, CFSTR("OpenCL.framework"), false);
	if (url != NULL) {
		framework = CFBundleCreate(kCFAllocatorSystemDefault, url);
		if (framework) {
			if (!CFBundleLoadExecutable(framework)) {
				CFRelease(framework);
				framework = NULL;
			}
		}
		CFRelease(url);
	}
	CFRelease(frameworksURL);
	return framework;
}

static void* clint_opencl_sym(void *dll, const char *sym)
{
	CFStringRef cf_sym = CFStringCreateWithCString(kCFAllocatorDefault, sym, kCFStringEncodingASCII);
	void *ptr = NULL;
	if (cf_sym != NULL) {
		ptr = CFBundleGetFunctionPointerForName((CFBundleRef)dll, cf_sym);
		CFRelease(cf_sym);
	}
	return ptr;
}

static void clint_opencl_unload(void *dll)
{
	CFBundleUnloadExecutable((CFBundleRef)dll);
	CFRelease((CFBundleRef)dll);
}

#ifdef __cplusplus
extern "C"
#endif
void NSLogv(CFStringRef, va_list);

static void clint_log(const char *fmt, ...)
{
	CFStringRef str;
	va_list ap;
	va_start(ap, fmt);
	str = CFStringCreateWithCString(kCFAllocatorDefault, fmt, kCFStringEncodingASCII);
	if (str) {
		NSLogv(str, ap);
		CFRelease(str);
	}
	va_end(ap);
}

#elif defined(WIN32)

#include <windows.h>
#include <tchar.h>

static void* clint_opencl_load(void)
{
	return LoadLibraryA("OpenCL.dll");
}

static void* clint_opencl_sym(void *dll, const char *sym)
{
	return (void*)GetProcAddress((HINSTANCE)dll, sym);
}

static void clint_opencl_unload(void *dll)
{
	FreeLibrary((HINSTANCE)dll);
}

#else

static void* clint_opencl_load(void)
{
	return dlopen("OpenCL.so", RTLD_LAZY | RTLD_LOCAL);
}

static void* clint_opencl_sym(void *dll, const char *sym)
{
	return dlsym(dll, sym);
}

static void clint_opencl_unload(void *dll)
{
	dlclose(dll);
}

static void clint_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#endif

#define ERR(a) case a: return #a;

const char *clErrorString(cl_int err)
{
	switch (err) {
	case CL_SUCCESS:
		return NULL;
	ERR(CL_DEVICE_NOT_FOUND);
	ERR(CL_DEVICE_NOT_AVAILABLE);
	ERR(CL_COMPILER_NOT_AVAILABLE);
	ERR(CL_MEM_OBJECT_ALLOCATION_FAILURE);
	ERR(CL_OUT_OF_RESOURCES);
	ERR(CL_OUT_OF_HOST_MEMORY);
	ERR(CL_PROFILING_INFO_NOT_AVAILABLE);
	ERR(CL_MEM_COPY_OVERLAP);
	ERR(CL_IMAGE_FORMAT_MISMATCH);
	ERR(CL_IMAGE_FORMAT_NOT_SUPPORTED);
	ERR(CL_BUILD_PROGRAM_FAILURE);
	ERR(CL_MAP_FAILURE);
#ifdef CL_MISALIGNED_SUB_BUFFER_OFFSET
	ERR(CL_MISALIGNED_SUB_BUFFER_OFFSET);
#endif
#ifdef CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST
	ERR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
#endif
	ERR(CL_INVALID_VALUE);
	ERR(CL_INVALID_DEVICE_TYPE);
	ERR(CL_INVALID_PLATFORM);
	ERR(CL_INVALID_DEVICE);
	ERR(CL_INVALID_CONTEXT);
	ERR(CL_INVALID_QUEUE_PROPERTIES);
	ERR(CL_INVALID_COMMAND_QUEUE);
	ERR(CL_INVALID_HOST_PTR);
	ERR(CL_INVALID_MEM_OBJECT);
	ERR(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
	ERR(CL_INVALID_IMAGE_SIZE);
	ERR(CL_INVALID_SAMPLER);
	ERR(CL_INVALID_BINARY);
	ERR(CL_INVALID_BUILD_OPTIONS);
	ERR(CL_INVALID_PROGRAM);
	ERR(CL_INVALID_PROGRAM_EXECUTABLE);
	ERR(CL_INVALID_KERNEL_NAME);
	ERR(CL_INVALID_KERNEL_DEFINITION);
	ERR(CL_INVALID_KERNEL);
	ERR(CL_INVALID_ARG_INDEX);
	ERR(CL_INVALID_ARG_VALUE);
	ERR(CL_INVALID_ARG_SIZE);
	ERR(CL_INVALID_KERNEL_ARGS);
	ERR(CL_INVALID_WORK_DIMENSION);
	ERR(CL_INVALID_WORK_GROUP_SIZE);
	ERR(CL_INVALID_WORK_ITEM_SIZE);
	ERR(CL_INVALID_GLOBAL_OFFSET);
	ERR(CL_INVALID_EVENT_WAIT_LIST);
	ERR(CL_INVALID_EVENT);
	ERR(CL_INVALID_OPERATION);
	ERR(CL_INVALID_GL_OBJECT);
	ERR(CL_INVALID_BUFFER_SIZE);
	ERR(CL_INVALID_MIP_LEVEL);
	ERR(CL_INVALID_GLOBAL_WORK_SIZE);
#ifdef CL_INVALID_PROPERTY
	ERR(CL_INVALID_PROPERTY);
#endif
#if defined(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
	ERR(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
#elif defined(CL_INVALID_GL_CONTEXT_APPLE)
	ERR(CL_INVALID_GL_CONTEXT_APPLE);
#endif
#if defined(CL_PLATFORM_NOT_FOUND_KHR)
	ERR(CL_PLATFORM_NOT_FOUND_KHR);
#endif
	default:
		return "Unknown";
	}
}

#include "clint_opencl_funcs.c"
