LIBNAME = OpenCL.framework/Versions/A/OpenCL
BUILD_DIR := $(shell name="build"; echo $$name; test -d $$name || mkdir $$name >/dev/null 2>&1)
CFLAGS = -Wall -O2 -arch i386 -arch x86_64 -I$(BUILD_DIR)

all:: $(BUILD_DIR)/$(LIBNAME)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(BUILD_DIR)/clint_opencl_funcs.c: scripts/gensource.py
	@rm -f $@
	scripts/gensource.py /System/Library/Frameworks/OpenCL.framework/Headers/{cl.h,cl_ext.h,cl_gl.h,cl_gl_ext.h} > $@

src/clint.c: $(BUILD_DIR)/clint_opencl_funcs.c

$(BUILD_DIR)/$(LIBNAME): $(BUILD_DIR)/clint.o
	@mkdir -p `dirname $@`
	@(LIBDIR=`dirname $@`; rm -f `dirname "$$LIBDIR"`/Current ; ln -sf `basename "$$LIBDIR"` `dirname "$$LIBDIR"`/Current)
	$(CC) $(CFLAGS) -bundle -o $@ $^ -framework CoreFoundation -framework Carbon -framework Foundation
