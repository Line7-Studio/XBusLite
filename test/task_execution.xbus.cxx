#include <XBus.hxx>

auto client_python_source = R"(

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

)";

bool init_client(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    XBus::Python::Eval(client_python_source);
    return true;
}

XBUS_REGISTE_CLIENT("Client", init_client);
