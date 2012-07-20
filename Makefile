#LIBNAME = OpenCL.framework/Versions/A/OpenCL
LIBNAME = libClint.dylib
BUILD_DIR := $(shell name="build"; echo $$name; test -d $$name || mkdir $$name >/dev/null 2>&1)
CFLAGS = -Wall -O0 -g -arch i386 -arch x86_64 -I$(BUILD_DIR) -Isrc

all:: $(BUILD_DIR)/$(LIBNAME) $(BUILD_DIR)/clinfo

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(BUILD_DIR)/clint_opencl_funcs.c $(BUILD_DIR)/clint_opencl_types.c $(BUILD_DIR)/clint_opencl_types.h: scripts/gensource.py
	@rm -f $@
	scripts/gensource.py /System/Library/Frameworks/OpenCL.framework/Headers/{cl.h,cl_ext.h,cl_gl.h,cl_gl_ext.h} -o ${BUILD_DIR}

$(BUILD_DIR)/clint_opencl_funcs.o: $(BUILD_DIR)/clint_opencl_funcs.c
$(BUILD_DIR)/clint_opencl_types.o: $(BUILD_DIR)/clint_opencl_types.c

$(BUILD_DIR)/$(LIBNAME): $(BUILD_DIR)/clint_opencl_funcs.o $(BUILD_DIR)/clint_opencl_types.o $(BUILD_DIR)/clint.o $(BUILD_DIR)/clint_config.o $(BUILD_DIR)/clint_data.o $(BUILD_DIR)/clint_log.o $(BUILD_DIR)/clint_thread.o
	$(CC) $(CFLAGS) -dynamiclib -compatibility_version 1.0.0 -current_version 1.0.0 -o $@ $^ -framework OpenCL -framework Foundation

#$(BUILD_DIR)/$(LIBNAME): $(BUILD_DIR)/clint_opencl_funcs.o $(BUILD_DIR)/clint_opencl_types.o $(BUILD_DIR)/clint.o $(BUILD_DIR)/clint_config.o $(BUILD_DIR)/clint_data.o $(BUILD_DIR)/clint_log.o $(BUILD_DIR)/clint_thread.o
#	@mkdir -p `dirname $@`
#	@(LIBDIR=`dirname $@`; rm -f `dirname "$$LIBDIR"`/Current ; ln -sf `basename "$$LIBDIR"` `dirname "$$LIBDIR"`/Current)
#	$(CC) $(CFLAGS) -dynamiclib -compatibility_version 1.0.0 -current_version 1.0.0 -o $@ $^ -framework Carbon -framework Foundation
#	@(LIBDIR=`dirname $@`; VERSDIR=`dirname "$$LIBDIR"`; TOPDIR=`dirname "$$VERSDIR"`; LIB=`basename $@`; rm -f "$$TOPDIR/$$LIB" ; ln -sf "Versions/Current/$$LIB" "$$TOPDIR/")

$(BUILD_DIR)/clinfo: $(BUILD_DIR)/clint_opencl_types.o $(BUILD_DIR)/clint_config.o $(BUILD_DIR)/clint_data.o $(BUILD_DIR)/clint_log.o $(BUILD_DIR)/clint_thread.o $(BUILD_DIR)/clinfo.o
	$(CC) $(CFLAGS) -o $@ $^ -framework Carbon -framework Foundation -framework OpenCL
