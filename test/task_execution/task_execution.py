import xbus

@xbus.declare.function
def Return_None():
    return


@xbus.declare.function
def Return_Text():
    return 'hello xbus'


@xbus.declare.function
def Return_Number_I():
    return 1


@xbus.declare.function
def Return_Number_F():
    return 3.14


@xbus.declare.function
def Return_Array():
    return [0, 1, 2, '3', [4, 5, 6.0], -7]


@xbus.declare.function
def Echo_One(arg_1):
    return arg_1


@xbus.declare.function
def Echo_Two_Swap(arg_1, arg_2):
    return arg_2, arg_1

