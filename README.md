CLIntercept inserts itself between a program and the OpenCL library. It can log OpenCL calls or errors, time expensive functions, modify the devices or extensions returned to the program, or track OpenCL objects.

Tracking OpenCL objects also allows CLIntercept to detect some common programming errors, such as leaks, mismatched calls to retain/release, situations that are not thread safe, and errors with mapped images or buffers, including buffer overwrites. Any of these can be missed by the OpenCL implementation and led to incorrect behavior or even crash the system.
