#!/usr/bin/env python

import os
import re
import string
import sys

pat_func = re.compile(r'(extern\s+CL_API_ENTRY\s+[^;]*\s+CL_API_SUFFIX__VERSION[_0-9]*\s*;)')
pat_extern = re.compile(r'extern\s+')
pat_name = re.compile(r'CL_API_CALL\s+(\w+)')
pat_func_before_args = re.compile(r'^.*CL_API_CALL\s+\w+\(')
pat_args = re.compile(r'(([a-zA-Z0-9_* ]+)\s+(\w+)\s*[,)])|((\w+(\s*[*])?\s*\(\s*(CL_CALLBACK)?\s*[*]\s*\w+\s*\)\s*\([^)]*\))\s*[,)])')
pat_func_ptr = re.compile(r'((CL_CALLBACK)?\s*[*])\s*(\w+)')
pat_return = re.compile(r'CL_API_ENTRY\s+(\w+(\s*[*])?)\s+CL_API_CALL')
pat_suffix = re.compile(r'\s+CL_API_SUFFIX__VERSION[_0-9]*')
pat_comment = re.compile(r'/[*]\s*(\w*).*?[*]/')

def pointer_name(name):
    return 'clint_' + name + '_ptr'

def typedef_name(name):
    return 'CLINT_' + name.upper() + '_FN'

def gen_prefix(out):
    out.write('#ifdef __cplusplus\n')
    out.write('extern "C" {\n')
    out.write('#endif\n')

def gen_postfix(out):
    out.write('#ifdef __cplusplus\n')
    out.write('}\n')
    out.write('#endif\n')

def gen_pointers(out, f):
    proto, name, r, args, core = f
    if core:
        out.write("static %s %s = NULL;\n" % (typedef_name(name), pointer_name(name)))

def gen_lookup(out, f):
    proto, name, r, args, core = f
    if core:
        out.write('\t\t\t%s = (%s)clint_opencl_sym(clint_dll, "%s");\n' % (pointer_name(name), typedef_name(name), name))

def gen_typedef(out, f):
    proto, name, r, args, core = f
    proto = pat_extern.sub('', proto)
    proto = pat_suffix.sub('', proto)
    proto = pat_name.sub('(CL_API_CALL *%s)' % typedef_name(name), proto)
    out.write("typedef %s\n" % proto)

gen_format_str_map = {
    'int': '%d',
    'long': '%ld',
    'cl_int': '%d',
    'cl_long': '%ld',
    'cl_uint': '%u',
    'cl_ulong': '%lu',
    'size_t': '%u',
    'cl_platform_id': '%p',
    'cl_device_id': '%p',
    'cl_context': '%p',
    'cl_command_queue': '%p',
    'cl_mem': '%p',
    'cl_program': '%p',
    'cl_kernel': '%p',
    'cl_event': '%p',
    'cl_sampler': '%p',
    'cl_platform_info': '%p',
    'cl_float': '%f',
    'cl_bool': '%u',
    'cl_bitfield': '%lu',
    'cl_device_type': '%lu',
    'cl_platform_info': '%u',
    'cl_device_info': '%u',
    'cl_device_fp_config': '%lu',
    'cl_device_mem_cache_type': '%u',
    'cl_device_local_mem_type': '%u',
    'cl_device_exec_capabilities': '%lu',
    'cl_command_queue_properties': '%lu',
    'cl_context_properties': '%p',
    'cl_context_info': '%u',
    'cl_command_queue_info': '%u',
    'cl_channel_order': '%u',
    'cl_channel_type': '%u',
    'cl_mem_flags': '%lu',
    'cl_mem_object_type': '%u',
    'cl_mem_info': '%u',
    'cl_image_info': '%u',
    'cl_buffer_create_type': '%u',
    'cl_addressing_mode': '%u',
    'cl_filter_mode': '%u',
    'cl_sampler_info': '%u',
    'cl_map_flags': '%lu',
    'cl_program_info': '%u',
    'cl_program_build_info': '%u',
    'cl_build_status': '%d',
    'cl_kernel_info': '%u',
    'cl_kernel_work_group_info': '%u',
    'cl_event_info': '%u',
    'cl_command_type': '%u',
    'cl_profiling_info': '%u',
    'cl_GLuint': '%u',
    'cl_GLint': '%d',
    'cl_GLenum': '%u',
    'cl_gl_object_type': '%u',
    'cl_gl_texture_info': '%u',
    'cl_gl_platform_info': '%u',
}

def gen_format_str(t):
    t = string.strip(t)
    if t == 'const char *':
        return '%s'
    if '*' in t or '(' in t:
        return '%p'
    return gen_format_str_map[t]

def gen_func(out, f):
    proto, name, r, args, core = f
    proto = pat_comment.sub(r'\1', proto)
    fmt = name + '(' + string.join(map(lambda a: a[1] + '=' + gen_format_str(a[0]), args), ", ") + ')'
    call_args = string.join(map(lambda a: a[1], args), ", ")
    out.write(proto[:-1] + "\n")
    out.write("{\n")
    if r != 'void':
        out.write("\t%s retval;\n" % r)
    do_errcode = (r == 'cl_int' or (args and args[-1][0] in ('cl_int *', 'int *')))
    if r != 'cl_int' and do_errcode:
        out.write("\tcl_int errcode_local;\n")
    out.write("\tclint_init();\n")
    out.write('\tclint_log(%s);\n' % string.join(['"%s"' % fmt] + map(lambda a: a[1], args), ", "))
    if r != 'cl_int' and do_errcode:
        out.write("\tif (%s == NULL) %s = &errcode_local;\n" % (args[-1][1], args[-1][1]))
    if core:
        call_str = pointer_name(name)
    else:
        call_str = '((%s)%s("%s"))' % (typedef_name(name), pointer_name('clGetExtensionFunctionAddress'), name)
    if do_errcode:
        errcode = ((r == 'cl_int') and 'retval') or '*'+args[-1][1]
        out.write('\tif (%s != CL_SUCCESS) clint_log("ERROR in %s: %%s", clErrorString(%s));\n' % (errcode, name, errcode))
    if r == 'void':
        out.write("\t%s(%s);" % (call_str, call_args))
    else:
        out.write("\tretval = %s(%s);\n" % (call_str, call_args))
    if r != 'void' or args:
        out_args = filter(lambda x: '*' in x[0] and (not 'const' in x[0]) and (x[0] != 'void *') and (x[0] != 'cl_image_format *') and (x[1] != 'errcode_ret'), args)
        if r != 'void':
            out_args = [(r, 'retval')] + out_args
        out_fmt = name + ' returned ' + string.join(map(lambda a: ((a[1] == 'retval') and gen_format_str(a[0])) or (a[1] + '=' + gen_format_str(string.strip(a[0][:-1]))), out_args), " ")
        out.write('\tclint_log(%s);\n' % string.join(['"%s"' % out_fmt] + map(lambda a: ((a[1] == 'retval') and 'retval') or (a[1]+' ? '+'*'+a[1]+' : 0'), out_args), ", "))
    if r != 'void':
        out.write("\treturn retval;\n")
    out.write("}\n")
    out.write("\n")

funcs = []

for filename in sys.argv:
    file = open(filename, 'r')
    text = file.read()
    protos = pat_func.findall(text)
    for proto in protos:
        name = pat_name.search(proto).group(1)
        r = pat_return.search(proto).group(1)
        argStr = pat_func_before_args.sub('', proto)
        argStr = pat_suffix.sub('', argStr)[:-1]
        argStr = pat_comment.sub(r'\1', argStr)
        matches = pat_args.findall(argStr)
        args = []
        for a in matches:
            func_ptr = a[4]
            if func_ptr:
                args.append((pat_func_ptr.sub(r'\3', func_ptr), pat_func_ptr.search(func_ptr).group(3)))
            else:
                args.append((string.strip(a[1]), a[2]))
        funcs.append([proto, name, r, args, os.path.basename(filename) == 'cl.h'])

out = sys.stdout

gen_prefix(out)
out.write("\n")
for f in funcs:
    gen_typedef(out, f)
out.write("\n")
for f in funcs:
    gen_pointers(out, f)
out.write("\n")
out.write("static void* clint_dll = NULL;\n")
out.write("static void clint_init(void)\n")
out.write("{\n")
out.write("\tif (clint_dll == NULL) {\n")
out.write("\t\tclint_dll = clint_opencl_load();\n")
out.write("\t\tif (clint_dll == NULL) {\n")
for f in funcs:
    gen_lookup(out, f)
out.write("\t\t}\n")
out.write("\t}\n")
out.write("}\n")
out.write("\n")
for f in funcs:
    gen_func(out, f)
out.write("\n")
gen_postfix(out)
