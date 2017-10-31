#include <XBus.hxx>


void fun_void_void()
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_void_void", fun_void_void);


void fun_void_int(int value)
{
    printf("%s %s %d\n", __FILE__, __FUNCTION__, value);
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_void_int", fun_void_int);


void fun_void_float(float value)
{
    printf("%s %s %f\n", __FILE__, __FUNCTION__, value);
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_void_float", fun_void_float);


void fun_echo_void()
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    return;
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_echo_void", fun_echo_void);


int fun_echo_int(int value)
{
    printf("%s %s %d\n", __FILE__, __FUNCTION__, value);
    return value;
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_echo_int", fun_echo_int);


float fun_echo_float(float value)
{
    printf("%s %s %f\n", __FILE__, __FUNCTION__, value);
    return value;
}
XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION("fun_echo_float", fun_echo_float);




bool init_client(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    XBus::Python::Eval(XBus::EmbededSourceLoader(":EmbededSource"));

    return true;

}

XBUS_REGISTE_CLIENT("Client", init_client);

