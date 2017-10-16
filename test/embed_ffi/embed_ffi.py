import xbus
import time
import ctypes



address = __fetch_embedded_function_address__('fun_void_void')
the_fun_void_void = ctypes.CFUNCTYPE(None)(address)

address = __fetch_embedded_function_address__('fun_void_int')
the_fun_void_int = ctypes.CFUNCTYPE(None)(address)

address = __fetch_embedded_function_address__('fun_void_float')
the_fun_void_float = ctypes.CFUNCTYPE(None)(address)


@xbus.declare.function
def fun_void_void():
    return the_fun_void_void()


@xbus.declare.function
def fun_void_int(value):
    return the_fun_void_int()


@xbus.declare.function
def fun_void_float(value):
    return the_fun_void_float()


address = __fetch_embedded_function_address__('fun_echo_void')
the_fun_echo_void = ctypes.CFUNCTYPE(None)(address)

address = __fetch_embedded_function_address__('fun_echo_int')
the_fun_echo_int = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)(address)

address = __fetch_embedded_function_address__('fun_echo_float')
the_fun_echo_float = ctypes.CFUNCTYPE(ctypes.c_float, ctypes.c_float)(address)


@xbus.declare.function
def fun_echo_void():
    return the_fun_echo_void()


@xbus.declare.function
def fun_echo_int(value):
    print("fun_echo_int")
    return the_fun_echo_int(value)


@xbus.declare.function
def fun_echo_float(value):
    return the_fun_echo_float(value)

