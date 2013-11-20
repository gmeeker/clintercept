#!/usr/bin/env python

import os
import re
import string
import sys

pat_func = re.compile(r'(extern\s+CL_API_ENTRY\s+(?:CL_[A-Z]*_PREFIX__VERSION[_0-9]*_DEPRECATED\s*)?[^;]*\s+CL_[A-Z]*_SUFFIX__VERSION[_0-9]*(?:_DEPRECATED)?\s*;)')
pat_extern = re.compile(r'extern\s+')
pat_name = re.compile(r'CL_API_CALL\s+(\w+)')
pat_func_before_args = re.compile(r'^.*CL_API_CALL\s+\w+\(')
pat_args = re.compile(r'(([a-zA-Z0-9_* ]+)\s+(\w+)\s*[,)])|((\w+(\s*[*])?\s*\(\s*(CL_CALLBACK)?\s*[*]\s*\w+\s*\)\s*\([^)]*\))\s*[,)])')
pat_func_ptr = re.compile(r'((CL_CALLBACK)?\s*[*])\s*(\w+)')
pat_return = re.compile(r'CL_API_ENTRY\s+(?:CL_[A-Z]*_PREFIX__VERSION[_0-9]*_DEPRECATED\s*)?(\w+(\s*[*])?)\s+CL_API_CALL')
pat_suffix = re.compile(r'\s+CL_[A-Z]*_SUFFIX__VERSION[_0-9A-Z]*')
pat_comment = re.compile(r'/[*]\s*(\w*).*?[*]/')
pat_c_cpp_comment = re.compile(r'\s*(/[*].*[*]/)|(//.*\n?)')
pat_type_comment = re.compile(r'\s*/[*]\s*(Additional\s+)?(cl_[a-z0-9_]+)\s*.*[*]/')
pat_type_cpp_comment = re.compile(r'\s*//\s*(Additional\s+)?(cl_[a-z0-9_]+)\s*.*\n?')
pat_err = re.compile(r'#define\s+(CL_[A-Za-z0-9_]+)\s+(-?[0-9]+)')
pat_define = re.compile(r'#define\s+(CL_[A-Za-z0-9_]+)\s+((0x[0-9A-Fa-f]+)|(-?[0-9]+))')
pat_bitfield = re.compile(r'#define\s+(CL_[A-Za-z0-9_]+)\s+(\([0-9]+\s*<<\s*[0-9]+\))')
pat_struct = re.compile(r'}\s*(cl_[a-z0-9_]+)\s*;')

def pointer_name(name):
    return 'clint_' + name + '_ptr'

def typedef_name(name):
    return 'CLINT_' + name.upper() + '_FN'

def remove_prefix(s, sub):
    if s[:len(sub)] == sub:
        return s[len(sub):]
    return s

def has_prefix(s, sub):
    return s[:len(sub)] == sub

def gen_top(out):
    out.write('/* AUTOMATICALLY GENERATED: DO NO EDIT */\n')
    out.write('#include "clint_data.h"\n')
    out.write('#include "clint_log.h"\n')

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
        out.write('static %s %s = NULL;\n' % (typedef_name(name), pointer_name(name)))

def gen_define_pointers(out, f):
    proto, name, r, args, core = f
    if core:
        out.write('#define %s %s\n' % (pointer_name(name), name))

def gen_interpose(out, f):
    proto, name, r, args, core = f
    if core:
        out.write('\t{ clint_%s, %s },\n' % (name, name))

def gen_lookup(out, f):
    proto, name, r, args, core = f
    if core:
        out.write('\t\t\t%s = (%s)clint_opencl_sym(clint_dll, "%s");\n' % (pointer_name(name), typedef_name(name), name))

def gen_typedef(out, f):
    proto, name, r, args, core = f
    proto = pat_extern.sub('', proto)
    proto = pat_suffix.sub('', proto)
    proto = pat_name.sub('(CL_API_CALL *%s)' % typedef_name(name), proto)
    out.write('typedef %s\n' % proto)

def gen_type_name(t):
    return remove_prefix(t, 'cl_')

def gen_type_arg(t):
    if t == 'error':
        return 'cl_int'
    elif t == 'token':
        return 'cl_uint'
    elif t == 'execution_status':
        return 'cl_int'
    else:
        return t

def is_type_const(t):
    return has_prefix(t, 'const ')

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
#    'cl_platform_info': '%p',
    'cl_float': '%f',
    'cl_bool': '%u',
    'cl_bitfield': '%lu',
#    'cl_device_type': '%lu',
#    'cl_platform_info': '%u',
#    'cl_device_info': '%u',
#    'cl_device_fp_config': '%lu',
#    'cl_device_mem_cache_type': '%u',
#    'cl_device_local_mem_type': '%u',
#    'cl_device_exec_capabilities': '%lu',
#    'cl_command_queue_properties': '%lu',
#    'cl_context_properties': '%p',
#    'cl_context_info': '%u',
#    'cl_command_queue_info': '%u',
#    'cl_channel_order': '%u',
#    'cl_channel_type': '%u',
#    'cl_mem_flags': '%lu',
#    'cl_mem_object_type': '%u',
#    'cl_mem_info': '%u',
#    'cl_image_info': '%u',
#    'cl_buffer_create_type': '%u',
#    'cl_addressing_mode': '%u',
#    'cl_filter_mode': '%u',
#    'cl_sampler_info': '%u',
#    'cl_map_flags': '%lu',
#    'cl_program_info': '%u',
#    'cl_program_build_info': '%u',
#    'cl_build_status': '%d',
#    'cl_kernel_info': '%u',
#    'cl_kernel_work_group_info': '%u',
#    'cl_event_info': '%u',
#    'cl_command_type': '%u',
#    'cl_profiling_info': '%u',
    'cl_GLuint': '%u',
    'cl_GLint': '%d',
    'cl_GLenum': '%u',
    'cl_gl_object_type': '%u',
#    'cl_gl_texture_info': '%u',
#    'cl_gl_platform_info': '%u',
    'cl_GLsync': '%p',
    'va_list': '%p',
}

gen_format_struct_map = {
    'cl_image_format': (('cl_channel_order', 'image_channel_order'),
                        ('cl_channel_type', 'image_channel_data_type')),
    'cl_image_desc': (('cl_mem_object_type', 'image_type'),
                      ('size_t', 'image_width'),
                      ('size_t', 'image_height'),
                      ('size_t', 'image_depth'),
                      ('size_t', 'image_array_size'),
                      ('size_t', 'image_row_pitch'),
                      ('size_t', 'image_slice_pitch'),
                      ('cl_uint', 'num_mip_levels'),
                      ('cl_uint', 'num_samples'),
                      ('cl_mem', 'buffer')),
    'cl_buffer_region': (('size_t', 'origin'),
                         ('size_t', 'size'))
}

def append_type_map(typeMap, key, value):
    if key == 'cl_bool':
        return
    if key in typeMap:
        if not value[0] in map(lambda x: x[0], typeMap[key]):
            if key == 'token':
                # Avoid duplicates
                if value[1] in map(lambda x: x[1], typeMap[key]):
                    if string.split(value[0], '_')[0] in ('AMD','KHR','NV'):
                        return
                    else:
                        typeMap[key] = filter(lambda x,v=value[1]: x[1] != v, typeMap[key])
            typeMap[key].append(value)
    else:
        typeMap[key] = [value]

def fix_type_name(type_name):
    if has_prefix(type_name, 'cl_d3d'):
        # fix errors in the comments that should use _khr
        if 'device_source' in type_name or 'device_set' in type_name:
            if type_name[-3:] == '_nv':
                return type_name[:-3] + '_khr'
            if type_name[-4:] != '_khr':
                return type_name + '_khr'
    # typos
    if type_name == "cl_kernel_arg_type_qualifer":
        return "cl_kernel_arg_type_qualifier"
    return type_name

def gen_format_struct_name(name):
    name = string.strip(name)
    if name[-1] == '*':
        name = name[:-1]
    name = remove_prefix(name, 'const ')
    name = string.strip(name)
    return name

def gen_format_str(t, typeMap, funcName, inout):
    t = string.strip(t)
    if t == 'cl_int' and funcName == 'clSetUserEventStatus':
        t = 'execution_status'
    if (gen_format_struct_name(t) in gen_format_struct_map) and (inout != 0 or is_type_const(t)) and (inout != 1 or not is_type_const(t)):
        return '%s'
    if t in typeMap:
        return '%s'
    if t == 'const char *':
        return '%s'
    if '*' in t or '(' in t:
        if inout == 1 and '*' in t and (not '(' in t) and gen_format_struct_name(t) != 'void':
            return gen_format_str(gen_format_struct_name(t), typeMap, funcName, inout)
        return '%p'
    return gen_format_str_map[t]

def gen_format_arg(t, name, typeMap, funcName, inout):
    t = string.strip(t)
    if t == 'cl_int' and funcName == 'clSetUserEventStatus':
        t = 'execution_status'
    if (gen_format_struct_name(t) in gen_format_struct_map) and (inout != 0 or is_type_const(t)) and (inout != 1 or not is_type_const(t)):
        return 'clint_string_%s(%s)' % (gen_type_name(gen_format_struct_name(t)), name)
    if inout == 1 and '*' in t and (not '(' in t) and gen_format_struct_name(t) != 'void':
        return '(%s ? *%s : 0)' % (name, name)
    if t == 'const char *':
        return 'clint_string_shorten(%s)' % name
    if not t in typeMap:
        return name
    return 'clint_string_%s(%s)' % (gen_type_name(t), name)

def gen_mem_sharing(funcName):
    if 'GL' in funcName:
        sharing = 'ClintObjectSharing_gl'
    elif 'D3D9' in funcName:
        sharing = 'ClintObjectSharing_d3d9'
    elif 'D3D10' in funcName:
        sharing = 'ClintObjectSharing_d3d10'
    elif 'D3D11' in funcName:
        sharing = 'ClintObjectSharing_d3d11'
    else:
        sharing = 'ClintObjectSharing_none'
    return sharing

gen_objects_list = string.split('cl_context cl_command_queue cl_mem cl_program cl_kernel cl_event cl_sampler cl_device_id')

def gen_check_input_arg(arg, args, funcName):
    if '*' in arg[0] and gen_format_struct_name(arg[0]) in gen_objects_list:
        if not is_type_const(arg[0]):
            return None
        type_name = gen_type_name(gen_format_struct_name(arg[0]))
        i = args.index(arg)
        if i > 0 and args[i-1][0] in ('cl_uint', 'size_t') and has_prefix(args[i-1][1], 'num_'):
            return 'clint_check_input_%s(%s, %s)' % (type_name+'s', args[i-1][1], arg[1])
    if not arg[0] in gen_objects_list:
        return None
    if has_prefix(funcName, 'clRetain'):
        return 'clint_retain_%s(%s)' % (gen_type_name(arg[0]), arg[1])
    if has_prefix(funcName, 'clRelease'):
        return 'clint_release_%s(%s)' % (gen_type_name(arg[0]), arg[1])
    if arg[0] == 'cl_device_id':
        # Only clRetainDevice and clReleaseDevice require a subdevice.
        return None
    return 'clint_check_input_%s(%s)' % (gen_type_name(arg[0]), arg[1])

def gen_check_output_arg(arg, args, funcName, pointers_only=1):
    check_args = ()
    for i in (('cl_context', 'ClintObjectType_context'),
              ('cl_command_queue', 'ClintObjectType_command_queue'),
              ('cl_mem', 'ClintObjectType_mem'),
              ('cl_program', 'ClintObjectType_program'),
              ('cl_device_id', 'ClintObjectType_device')):
        check_args = filter(lambda a, i=i: a[0] == i[0], args)
        if check_args:
            check_args = (check_args[0][1], i[1])
            break
    if gen_format_struct_name(arg[0]) == 'cl_context':
        check_args = ('NULL', 'ClintObjectType_none')
    if gen_format_struct_name(arg[0]) == 'cl_device_id':
        # Only track subdevices.
        if funcName != 'clCreateSubDevices':
            return None
        check_args = ('NULL', 'ClintObjectType_none')
    if gen_format_struct_name(arg[0]) == 'cl_command_queue' and funcName == 'clCreateContextAndCommandQueueAPPLE':
        check_args = ('*'+args[-2][1], 'ClintObjectType_context')
    if gen_format_struct_name(arg[0]) == 'cl_mem':
        if has_prefix(funcName, 'clCreateFrom'):
            sharing = gen_mem_sharing(funcName)
        else:
            sharing = 'ClintObjectSharing_none'
        flag_args = filter(lambda a: a[0] == 'cl_mem_flags', args)
        if flag_args:
            check_args = tuple(check_args) + (flag_args[-1][1], sharing)
        image_format_args = filter(lambda a: a[0] == 'const cl_image_format *', args)
        if image_format_args:
            check_args = tuple(check_args) + (image_format_args[-1][1],)
        else:
            check_args = tuple(check_args) + ('NULL',)
    if '*' in arg[0] and gen_format_struct_name(arg[0]) in gen_objects_list:
        if is_type_const(arg[0]):
            return None
        type_name = gen_type_name(gen_format_struct_name(arg[0]))
        i = args.index(arg)
        if i > 0 and args[i-1][0] in ('cl_uint', 'size_t') and has_prefix(args[i-1][1], 'num_'):
            return 'clint_check_output_%s(%s)' % (type_name+'s', string.join((args[i-1][1], arg[1]) + tuple(check_args), ", "))
        return 'if (%s)\n\t\tclint_check_output_%s(%s)' % (arg[1], type_name, string.join(('*'+arg[1],) + tuple(check_args), ", "))
    if pointers_only:
        return None
    if not arg[0] in gen_objects_list:
        return None
    return 'clint_check_output_%s(%s)' % (gen_type_name(arg[0]), string.join((arg[1],) + tuple(check_args), ", "))

profile_funcs = ('clEnqueueNDRangeKernel', 'clEnqueueTask', 'clEnqueueNativeKernel')

def is_profile_all(name, args):
    return has_prefix(name, 'clEnqueue') and filter(lambda a: a[0] == 'cl_event *', args)

def gen_func_has_errcode(f):
    proto, name, r, args, core = f
    return (r == 'cl_int' or (args and args[-1][0] in ('cl_int *', 'int *')))

def gen_func_errcode(f):
    proto, name, r, args, core = f
    return ((r == 'cl_int') and 'retval') or '*'+args[-1][1]

def gen_custom_func_decl(out, f, typeMap):
    proto, name, r, args, core = f
    if is_profile_all(name, args):
        out.write('\tcl_event profile_event = NULL;\n')

def gen_custom_func_begin(out, f, typeMap):
    proto, name, r, args, core = f
    if 'Create' in name or 'Retain' in name or 'Release' in name:
        out.write('\tclint_opencl_enter();\n')
    if name == 'clSetKernelArg':
        out.write('\tclint_kernel_enter(%s);\n' % args[0][1])
    if ('Image' in name or 'Texture' in name) and not name in ('clGetSupportedImageFormats',):
        check = 'clint_get_config(CLINT_DISABLE_IMAGE)'
        if '3D' in name:
            check += ' || clint_get_config(CLINT_EMBEDDED)'
        elif name == 'clCreateImage':
            check += ' || (clint_get_config(CLINT_EMBEDDED) && (%s & CL_MEM_OBJECT_IMAGE3D) != 0)' % args[1][1]
        out.write('\tif (%s) {\n' % check)
        if r == 'cl_int':
            out.write('\t\tretval = CL_INVALID_OPERATION;\n')
        else:
            out.write('\t\t*%s = CL_INVALID_OPERATION;\n' % args[-1][1])
            out.write('\t\tretval = NULL;\n')
        out.write('\t} else /* okay to call real function */\n')
    if is_profile_all(name, args):
        config_value = 'CLINT_PROFILE_ALL'
        if name in profile_funcs:
            config_value = 'CLINT_PROFILE'
        arg = filter(lambda a: a[0] == 'cl_event *', args)[-1]
        out.write('\tif (clint_get_config(%s)) {\n' % config_value)
        out.write('\t\tif (%s == NULL) %s = &profile_event;\n' % (arg[1], arg[1]))
        out.write('\t}\n')
    if has_prefix(name, 'clCreate') and 'CommandQueue' in name:
        arg = filter(lambda a: a[0] == 'cl_command_queue_properties', args)
        if arg:
            arg = arg[-1]
            out.write('\tif (clint_get_config(CLINT_PROFILE))\n')
            out.write('\t\t%s |= CL_QUEUE_PROFILING_ENABLE;\n' % arg[1])
    if has_prefix(name, 'clEnqueueAcquire'):
        sharing = gen_mem_sharing(name)
        out.write('\tclint_acquire_shared_mems(%s, %s, %s);\n' % (args[1][1], args[2][1], sharing))
    if name in ('clBuildProgram', 'clCompileProgram'):
        arg = filter(lambda a: a[0] == 'const char *', args)[-1]
        out.write('\t\t%s = clint_modify_build_options(%s);\n' % (arg[1], arg[1]))
    if name == 'clCreateProgramWithSource':
        arg0 = filter(lambda a: a[0] == 'cl_uint', args)[-1]
        arg1 = filter(lambda a: a[0] == 'const char **', args)[-1]
        arg2 = filter(lambda a: a[0] == 'const size_t *', args)[-1]
        out.write('\t\t%s = clint_modify_program_sources(%s, %s, %s);\n' % (arg1[1], arg0[1], arg1[1], '&'+arg2[1]))
    if name == 'clCreateContextFromType':
        arg = filter(lambda a: a[0] == 'cl_device_type', args)[-1]
        out.write('\t\t%s = clint_modify_device_type(%s);\n' % (arg[1], arg[1]))
    elif has_prefix(name, 'clCreateContext'):
        arg0 = filter(lambda a: 'cl_context_properties' in a[0], args)[0]
        arg1 = filter(lambda a: 'cl_uint' == a[0], args)[0]
        arg2 = filter(lambda a: 'cl_device_id' in a[0], args)[0]
        out.write('\t%s = clint_modify_context_devices(%s, %s, %s);\n' % (arg2[1], arg0[1], '&'+arg1[1], arg2[1]))
    if has_prefix(name, 'clEnqueueRelease'):
        sharing = gen_mem_sharing(name)
        out.write('\tclint_release_shared_mems(%s, %s, %s);\n' % (args[1][1], args[2][1], sharing))

def gen_custom_func_exit(out, f, typeMap):
    proto, name, r, args, core = f
    if 'Create' in name or 'Retain' in name or 'Release' in name:
        out.write('\tclint_opencl_exit();\n')
    if name == 'clSetKernelArg':
        out.write('\tclint_kernel_exit(%s);\n' % args[0][1])
    if name == 'clEnqueueUnmapMemObject':
        arg = filter(lambda a: a[0] == 'cl_mem', args)[-1]
        out.write('\tclint_release_map(%s);\n' % arg[1])
    if has_prefix(name, 'clEnqueueMap'):
        arg0 = filter(lambda a: a[0] == 'cl_mem', args)[-1]
        arg1 = filter(lambda a: a[0] == 'cl_map_flags', args)[-1]
        arg2 = ('void *', 'retval')
        if name == 'clEnqueueMapImage':
            arg3 = filter(lambda a: 'size_t' in a[0], args)[1]
            arg4 = filter(lambda a: 'size_t' in a[0], args)[2]
            arg5 = filter(lambda a: 'size_t' in a[0], args)[3]
            out.write('\t%s = clint_retain_map_image(%s, %s, %s, %s, %s, %s);\n' % (arg2[1], arg0[1], arg1[1], arg2[1], arg3[1], arg4[1], arg5[1]))
        else:
            arg3 = filter(lambda a: a[0] == 'size_t', args)[1]
            out.write('\t%s = clint_retain_map(%s, %s, %s, %s);\n' % (arg2[1], arg0[1], arg1[1], arg2[1], arg3[1]))
        out.write('\tif (clint_get_config(CLINT_CHECK_MAPPING))\n\t\t%s = CL_TRUE;\n' % filter(lambda a: a[0] == 'cl_bool', args)[-1][1])
    if name in ('clGetPlatformInfo', 'clGetDeviceInfo'):
        if name == 'clGetPlatformInfo':
            param_name = 'CL_PLATFORM_EXTENSIONS'
        else:
            param_name = 'CL_DEVICE_EXTENSIONS'
        out.write('\tif (clint_get_config(CLINT_ENABLED) && %s == %s) {\n' % (args[1][1], param_name))
        out.write('\t\tclint_extensions_modify(%s, (char*)%s, %s);\n' % (args[2][1], args[3][1], args[4][1]))
        out.write('\t}\n')

        arg = ((name == 'clGetPlatformInfo') and 'CL_PLATFORM_PROFILE') or 'CL_DEVICE_PROFILE'
        # Don't worry about calling the real clGetPlatformInfo or clGetDeviceInfo
        # because EMBEDDED_PROFILE is longer than FULL_PROFILE.
        profile = 'EMBEDDED_PROFILE'
        out.write('\tif (clint_get_config(CLINT_EMBEDDED) && %s == %s) {\n' % (args[1][1], arg))
        out.write('\t\tif (%s != NULL)\n' % args[4][1])
        out.write('\t\t\t*%s = %d;\n' % (args[4][1], len(profile)+1))
        out.write('\t\tif (%s >= %d && %s != NULL)\n' % (args[2][1], len(profile)+1, args[3][1]))
        out.write('\t\t\tmemcpy(%s, \"%s\", %d);\n' % (args[3][1], profile, len(profile)+1))
        out.write('\t}\n')

        if name == 'clGetDeviceInfo':
            out.write('\tif (clint_get_config(CLINT_DISABLE_IMAGE) && %s > 0 && %s != NULL) {\n' % (args[2][1], args[3][1]))
            out.write('\t\tswitch (%s) {\n' % args[1][1])
            out.write('\t\tcase CL_DEVICE_IMAGE_SUPPORT:\n')
            out.write('\t\tcase CL_DEVICE_MAX_READ_IMAGE_ARGS:\n')
            out.write('\t\tcase CL_DEVICE_MAX_WRITE_IMAGE_ARGS:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE2D_MAX_WIDTH:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE2D_MAX_HEIGHT:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_WIDTH:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_HEIGHT:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_DEPTH:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:\n')
            out.write('\t\tcase CL_DEVICE_MAX_SAMPLERS:\n')
            out.write('\t\t\tmemset(%s, 0, %s);\n' % (args[3][1], args[2][1]))
            out.write('\t\t\tbreak;\n')
            out.write('\t\t}\n')
            out.write('\t} else if (clint_get_config(CLINT_EMBEDDED) && %s > 0 && %s != NULL) {\n' % (args[2][1], args[3][1]))
            out.write('\t\tswitch (%s) {\n' % args[1][1])
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_WIDTH:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_HEIGHT:\n')
            out.write('\t\tcase CL_DEVICE_IMAGE3D_MAX_DEPTH:\n')
            out.write('\t\t\tmemset(%s, 0, %s);\n' % (args[3][1], args[2][1]))
            out.write('\t\t\tbreak;\n')
            out.write('\t\t}\n')
            out.write('\t}\n')
    if is_profile_all(name, args):
        config_value = 'CLINT_PROFILE_ALL'
        if name in profile_funcs:
            config_value = 'CLINT_PROFILE'
        arg = filter(lambda a: a[0] == 'cl_event *', args)[-1]
        out.write('\tif (clint_get_config(%s))\n' % config_value)
        out.write('\t\t%s = clint_log_profile("%s", *%s);\n' % (gen_func_errcode(f), name, arg[1]))
        out.write('\tif (profile_event != NULL)\n')
        out.write('\t\tCLINTFUNC(clReleaseEvent)(profile_event);\n')

def gen_func(out, f, typeMap):
    proto, name, r, args, core = f
    proto = pat_comment.sub(r'\1', proto)
    proto = pat_suffix.sub('', proto)
    proto = proto.replace(name, 'F(%s)' % name)

    fmt = name + '(' + string.join(map(lambda a: a[1] + '=' + gen_format_str(a[0], typeMap, name, 0), args), ", ") + ')'
    call_args = string.join(map(lambda a: a[1], args), ", ")
    out.write(proto[:-1] + "\n")
    out.write('{\n')
    out.write('\tClintAutopool pool;\n')
    if r != 'void':
        out.write('\t%s retval;\n' % r)
    do_errcode = gen_func_has_errcode(f)
    if r != 'cl_int' and do_errcode:
        out.write('\tcl_int errcode_local;\n')
    gen_custom_func_decl(out, f, typeMap)
    out.write('\tclint_init();\n')
    out.write('\tclint_autopool_begin(&pool);\n')
    out.write('\tif (clint_get_config(CLINT_TRACE))\n')
    out.write('\t\tclint_log(%s);\n' % string.join(['"%s\\n"' % fmt] + map(lambda a: gen_format_arg(a[0], a[1], typeMap, name, 0), args), ", "))
    for a in args:
        check = gen_check_input_arg(a, args, name)
        if check:
            out.write('\t%s;\n' % check)
    if r != 'cl_int' and do_errcode:
        out.write('\tif (%s == NULL) %s = &errcode_local;\n' % (args[-1][1], args[-1][1]))
    gen_custom_func_begin(out, f, typeMap)
    if core:
        call_str = 'CLINTFUNC(%s)' % name
    else:
        call_str = '((%s)CLINTFUNC(%s)("%s"))' % (typedef_name(name), 'clGetExtensionFunctionAddress', name)
    if r == 'void':
        out.write('\t%s(%s);' % (call_str, call_args))
    else:
        out.write('\tretval = %s(%s);\n' % (call_str, call_args))
    gen_custom_func_exit(out, f, typeMap)
    if do_errcode:
        errcode = gen_func_errcode(f)
        out.write('\tif (%s != CL_SUCCESS && clint_get_config(CLINT_ERRORS)) {\n' % errcode)
        out.write('\t\tclint_log("ERROR in %s: %%s\\n", clint_string_error(%s));\n' % (name, errcode))
        out.write('\t\tclint_log_abort();\n')
        out.write('\t}\n')
    if r != 'void' or args:
        out_args = filter(lambda x: '*' in x[0] and (not 'const' in x[0]) and (x[0] != 'void *') and (x[1] != 'errcode_ret'), args)
        if r != 'void':
            out_args = [(r, 'retval')] + out_args
        out_fmt = name + ' returned ' + string.join(map(lambda a: ((a[1] == 'retval') and gen_format_str(a[0], typeMap, name, 1)) or (a[1] + '=' + gen_format_str(string.strip(a[0][:-1]), typeMap, name, 1)), out_args), " ")
        out.write('\tif (clint_get_config(CLINT_TRACE))\n')
        out.write('\t\tclint_log(%s);\n' % string.join(['"%s\\n"' % out_fmt] + map(lambda a: ((a[1] == 'retval') and 'retval') or gen_format_arg(a[0], a[1], typeMap, name, 1), out_args), ", "))
    checks = ''
    if do_errcode:
        indent = '\t\t'
    else:
        indent = '\t'
    for a in args:
        check = gen_check_output_arg(a, args, name)
        if check:
            checks += '%s%s;\n' % (indent, check)
    check = gen_check_output_arg((r, 'retval'), args, name, 0)
    if check:
        checks += '%s%s;\n' % (indent, check)
    if checks:
        if do_errcode:
            errcode = gen_func_errcode(f)
            out.write('\tif (%s == CL_SUCCESS) {\n' % errcode)
            out.write(checks)
            out.write('\t}\n')
    out.write('\tclint_autopool_end(&pool);\n')
    if r != 'void':
        out.write('\treturn retval;\n')
    out.write('}\n')
    out.write('\n')

def scanFile(file, filename, funcs, typeMap, typeIncludes):
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

    type_name = None
    for line in string.split(text, '\n'):
        m = pat_struct.search(line)
        if m:
            v = m.group(1)
            if v in gen_format_struct_map:
                typeMap[v] = ''
        if type_name:
            m = pat_define.search(line)
            if not m:
                m = pat_bitfield.search(line)
            if m:
                v = (m.group(1), m.group(2))
                append_type_map(typeMap, type_name, v)
                if v[0] == 'CL_CONTEXT_PLATFORM':
                    # special case: belongs to cl_context_info and cl_context_properties on Mac
                    append_type_map(typeMap, 'cl_context_properties', v)
                if v[1][:2] == '0x' and string.atol(v[1], 16) >= 0x1000:
                    append_type_map(typeMap, 'token', v)
            elif not pat_c_cpp_comment.search(line):
                # end of type defines
                type_name = None
        else:
            m = pat_type_comment.search(line)
            if not m:
                m = pat_type_cpp_comment.search(line)
            if m and 'cl_mem flag - bitfield' in line:
                type_name = 'cl_mem_flags'
            elif m and not 'extension' in line:
                type_name = fix_type_name(m.group(2))
            elif '/* command execution status */' in line:
                type_name = 'execution_status'
            else:
                m = pat_err.search(line)
                if m:
                    v = (m.group(1), m.group(2))
                    if v[0] == 'CL_SUCCESS' or v[1][0] == '-':
                        append_type_map(typeMap, 'error', v)
                        continue
                m = pat_define.search(line)
                if m:
                    v = (m.group(1), m.group(2))
                    if v[1][:2] == '0x':
                        if 'CL_DEVICE' in v[0]:
                            append_type_map(typeMap, 'cl_device_info', v)
                        elif 'CL_IMAGE' in v[0]:
                            append_type_map(typeMap, 'cl_image_info', v)
                        elif 'CL_CGL' in v[0]:
                            append_type_map(typeMap, 'cl_gl_platform_info', v)
                        elif 'CL_COMMAND' in v[0]:
                            append_type_map(typeMap, 'cl_command_type', v)
                        elif 'CL_CONTEXT_PROPERTY' in v[0]:
                            append_type_map(typeMap, 'cl_context_properties', v)
                        elif 'CL_PROGRAM' in v[0]:
                            append_type_map(typeMap, 'cl_program_info', v)
                        elif 'CL_AFFINITY_DOMAIN' in v[0]:
                            append_type_map(typeMap, 'cl_device_affinity_domain', v)
                        elif v[0] in ('CL_CONTEXT_MEMORY_INITIALIZE_KHR',
                                      'CL_CONTEXT_TERMINATE_KHR'):
                            append_type_map(typeMap, 'cl_context_properties', v)
                        elif v[0] in ('CL_1RGB_APPLE', 'CL_BGR1_APPLE', 'CL_ABGR_APPLE'):
                            append_type_map(typeMap, 'cl_channel_order', v)
                        elif v[0] in ('CL_YCbYCr_APPLE', 'CL_CbYCrY_APPLE', 'CL_SFIXED14_APPLE', 'CL_BIASED_HALF_APPLE'):
                            append_type_map(typeMap, 'cl_channel_type', v)
                        elif 'CL_CONTEXT_OFFLINE_DEVICES_AMD' == v[0]:
                            append_type_map(typeMap, 'cl_context_info', v)
                        else:
                            sys.stderr.write('Ungrouped #define %s %s\n' % (m.group(1), m.group(2)))
                            continue
                        append_type_map(typeMap, 'token', v)
                m = pat_bitfield.search(line)
                if m:
                    v = (m.group(1), m.group(2))
                    if 'CL_MEM_USE' in v[0]:
                        append_type_map(typeMap, 'cl_mem_flags', v)
                    else:
                        sys.stderr.write('Ungrouped bitfield %s %s\n' % (m.group(1), m.group(2)))
            includeName = os.path.basename(filename)
            if type_name and includeName != 'cl.h' and not includeName in typeIncludes:
                typeIncludes.append(includeName)

def gen_type_header(file, typeMap, typeIncludes):
    file.write('#ifndef _CLINT_OPENCL_TYPES_H_\n')
    file.write('#define _CLINT_OPENCL_TYPES_H_\n\n')
    gen_top(file)
    if typeIncludes:
        file.write('\n')
        file.write('#define D3D10_IGNORE_SDK_LAYERS\n')
        file.write('#define D3D11_IGNORE_SDK_LAYERS\n')
        file.write('#ifdef __APPLE__\n')
        for i in typeIncludes:
            file.write('#include <OpenCL/%s>\n' % i)
        file.write('#else\n')
        for i in typeIncludes:
            file.write('#include <CL/%s>\n' % i)
        file.write('#endif\n')
    file.write('\n')
    gen_prefix(file)
    file.write('\n')
    types = typeMap.keys()
    types.sort()
    for t in types:
        if t in gen_format_struct_map:
            file.write('const char *clint_string_%s(const %s *v);\n' % (gen_type_name(t), gen_type_arg(t)))
        else:
            file.write('const char *clint_string_%s(%s v);\n' % (gen_type_name(t), gen_type_arg(t)))
    file.write('\n')
    gen_postfix(file)
    file.write('\n#endif\n')

def gen_type_source(file, typeMap):
    gen_top(file)
    file.write('#include "clint_opencl_types.h"\n')
    file.write('\n')
    file.write('#ifdef __APPLE__\n')
    file.write('\n')
    file.write('#include <OpenCL/cl_gl.h>\n')
    file.write('#include <OpenCL/cl_gl_ext.h>\n')
    file.write('#include <OpenCL/cl_ext.h>\n')
    file.write('\n')
    file.write('#else\n')
    file.write('\n')
    file.write('#include <CL/cl_gl.h>\n')
    file.write('#include <CL/cl_gl_ext.h>\n')
    file.write('#include <CL/cl_ext.h>\n')
    file.write('\n')
    file.write('#endif\n')
    file.write('\n')
    gen_prefix(file)
    file.write('\n')
    types = typeMap.keys()
    types.sort()
    for t in types:
        if t in gen_format_struct_map:
            continue
        file.write('const char *clint_string_%s(%s v)\n' % (gen_type_name(t), gen_type_arg(t)))
        file.write('{\n')
        if typeMap[t] and ('<<' in typeMap[t][0][1]):
            # bitfield
            file.write('\tconst char *s = "";\n')
            file.write('\t%s v0 = v;\n' % gen_type_arg(t))
            file.write('\tif (v == 0)\n')
            file.write('\t\treturn "0";\n')
            for i in typeMap[t]:
                file.write('\tif ((v & %s) != 0) { /* %s */\n' % i)
                file.write('\t\tv &= ~(%s);\n' % i[0])
                file.write('\t\ts = clint_string_join(s, "%s", " | ");\n' % i[0])
                file.write('\t}\n')
            file.write('\tif (v != 0)\n')
            file.write('\t\treturn clint_string_sprintf("Unknown %s 0x%%X", (unsigned int)v0);\n' % t)
            file.write('\treturn s;\n')
        else:
            file.write('\tswitch (v) {\n')
            for i in typeMap[t]:
                file.write('\tcase %s: /* %s */\n' % i)
                file.write('\t\treturn "%s";\n' % i[0])
            file.write('\tdefault:\n')
            file.write('\t\treturn clint_string_sprintf("Unknown %s 0x%%X", (unsigned int)v);\n' % t)
            file.write('\t}\n')
        file.write('}\n\n')
    types = typeMap.keys()
    types.sort()
    for t in types:
        if not t in gen_format_struct_map:
            continue
        fmt = string.join([gen_format_str(v[0], typeMap, '', -1) for v in gen_format_struct_map[t]], ',')
        args = string.join(['"%s"' % fmt] + [gen_format_arg(v[0], 'v->' + v[1], typeMap, '', -1) for v in gen_format_struct_map[t]], ', ')
        file.write('const char *clint_string_%s(const %s *v)\n' % (gen_type_name(t), gen_type_arg(t)))
        file.write('{\n')
        file.write('\tif (v == NULL)\n')
        file.write('\t\treturn "NULL";\n')
        file.write('\treturn clint_string_sprintf(%s);\n' % args)
        file.write('}\n\n')
    gen_postfix(file)

def gen_func_source(file, funcs, typeMap):
    file.write('#define CL_USE_DEPRECATED_OPENCL_1_0_APIS\n')
    file.write('#define CL_USE_DEPRECATED_OPENCL_1_1_APIS\n')
    file.write('\n')
    file.write('#if defined(WIN32)\n')
    file.write('#include <CL/cl_platform.h>\n')
    file.write('#undef CL_API_ENTRY\n')
    file.write('#define CL_API_ENTRY __declspec(dllexport)\n')
    file.write('#endif\n')
    file.write('\n')
    gen_top(file)
    file.write('\n')
    file.write('#include "clint.h"\n')
    file.write('#include "clint_config.h"\n')
    file.write('#include "clint_obj.h"\n')
    file.write('#include "clint_opencl_types.h"\n')
    file.write('\n')
    file.write('#include <string.h>\n')
    file.write('\n')
    gen_prefix(file)
    file.write('\n')
    file.write('#ifndef CL_DEVICE_IMAGE_MAX_BUFFER_SIZE\n')
    file.write('#define CL_DEVICE_IMAGE_MAX_BUFFER_SIZE 0x1040\n')
    file.write('#endif\n')
    file.write('#ifndef CL_DEVICE_IMAGE_MAX_ARRAY_SIZE\n')
    file.write('#define CL_DEVICE_IMAGE_MAX_ARRAY_SIZE 0x1041\n')
    file.write('#endif\n')
    file.write('\n')
    for f in funcs:
        gen_typedef(file, f)
    file.write('\n')
    file.write('#ifndef __APPLE__\n')
    for f in funcs:
        gen_pointers(file, f)
    file.write('#define F(a) a\n')
    file.write('#else /*__APPLE__*/\n')
    for f in funcs:
        gen_define_pointers(file, f)
    file.write('#define F(a) clint_##a\n')
    file.write('#endif /*__APPLE__*/\n')
    file.write('\n')
    file.write('#define CLINTFUNC(a) clint_##a##_ptr\n')
    file.write('\n')
    file.write('static cl_int clint_log_profile(const char *name, cl_event event)\n')
    file.write('{\n')
    file.write('\tcl_ulong start, end;\n')
    file.write('\tcl_int err;\n')
    file.write('\tdouble elapsed;\n')
    file.write('\terr = CLINTFUNC(clWaitForEvents)(1, &event);\n')
    file.write('\tif (!err) return err;\n')
    file.write('\terr = CLINTFUNC(clGetEventProfilingInfo)(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);\n')
    file.write('\tif (!err) return err;\n')
    file.write('\terr = CLINTFUNC(clGetEventProfilingInfo)(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);\n')
    file.write('\tif (!err) return err;\n')
    file.write('\telapsed = (double)(end - start) * 1.0e-9;\n')
    file.write('\tclint_log("PROFILE: %s %f\\n", name, elapsed);\n')
    file.write('\treturn err;\n')
    file.write('}\n')
    file.write('\n')
    file.write('static void* clint_dll = NULL;\n')
    file.write('static void clint_init(void)\n')
    file.write('{\n')
    file.write('\tif (clint_dll == NULL) {\n')
    file.write('\t\tclint_dll = clint_opencl_load();\n')
    file.write('\t\tif (clint_dll != NULL) {\n')
    file.write('#ifndef __APPLE__\n')
    for f in funcs:
        gen_lookup(file, f)
    file.write('#endif /*__APPLE__*/\n')
    file.write('\t\t\tclint_opencl_init();\n')
    file.write('\t\t}\n')
    file.write('\t}\n')
    file.write('}\n')
    file.write('\n')
    for f in funcs:
        gen_func(file, f, typeMap)
    file.write('\n')
    file.write('#ifdef __APPLE__\n')
    file.write('__attribute__ ((section("__DATA, __interpose"))) struct {\n')
    file.write('\tvoid *new_func;\n')
    file.write('\tvoid *old_func;\n')
    file.write('} clint_interpose_funcs[] = {\n')
    for f in funcs:
        gen_interpose(file, f)
    file.write('};\n')
    file.write('#endif /*__APPLE__*/\n')
    file.write('\n')
    gen_postfix(file)

funcs = []
typeMap = {}
typeIncludes = []

base = None
incdir = None

i = 1
while i < len(sys.argv):
    filename = sys.argv[i]
    i = i + 1
    if filename == '-i':
        incdir = sys.argv[i]
        if os.path.basename(incdir)[-10:] == '.framework':
            incdir = os.path.join(incdir, 'Headers')
        elif os.path.basename(incdir) != 'CL':
            incdir = os.path.join(incdir, 'CL')
        i = i + 1
        continue
    if filename == '-o':
        base = sys.argv[i]
        i = i + 1
        continue
    if incdir and not os.path.isabs(filename):
        filename = os.path.join(incdir, filename)
    if not os.path.exists(filename) and os.path.basename(filename) in ('cl_dx9_media_sharing.h', 'cl_d3d10.h', 'cl_d3d11.h'):
        continue
    file = open(filename, 'r')
    scanFile(file, filename, funcs, typeMap, typeIncludes)

out = sys.stdout

if base:
    out = open(os.path.join(base, 'clint_opencl_types.h'), 'w')
gen_type_header(out, typeMap, typeIncludes)

if base:
    out = open(os.path.join(base, 'clint_opencl_types.c'), 'w')
gen_type_source(out, typeMap)

if base:
    out = open(os.path.join(base, 'clint_opencl_funcs.c'), 'w')
gen_func_source(out, funcs, typeMap)
