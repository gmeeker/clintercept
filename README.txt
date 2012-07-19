To use the library:

Window:

Copy OpenCL.dll and ClintConfig.txt next to the application.
Edit the options in ClintConfig.txt, especially the log file location.

Mac OS X:

Override DYLD_FRAMEWORK_PATH to the install path and set any options in the environment.
You may also specify option in ClintConfig.txt.
Logging will use NSLog by default, but a log file can be specified.

DYLD_FRAMEWORK_PATH=/my/install/path/ CLINT_CHECK_ALL=1 ./clinfo

Linux:

LD_PRELOAD=/my/install/path/libOpenCL.so LD_LIBRARY_PATH=/my/install/path/ CLINT_CHECK_ALL=1 ./clinfo
