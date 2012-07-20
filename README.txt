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
