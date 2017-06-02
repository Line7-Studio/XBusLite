
import xbus


@xbus.declare.function
def fun_0():
    #print('fun_0 called')
    pass


@xbus.declare.function
def fun_1():
    #print('fun_1 called')
    return 'hello xbus'


@xbus.declare.function
def fun_2(args):
    #print('fun_2 called')
    return args


@xbus.declare.function
def fun_3(value: int):
    #print('fun_3 called')
    return value*2

