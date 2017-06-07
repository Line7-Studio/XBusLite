#include <XBus.hxx>

auto client_python_source = R"(

embed_module = __load_embeded_module__(':EmbededSource')

fun_0 = embed_module.fun_0
fun_1 = embed_module.fun_1
fun_2 = embed_module.fun_2
fun_3 = embed_module.fun_3

)";

bool init_client(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    XBus::Python::Eval(client_python_source);

    return true;

}

XBUS_REGISTE_CLIENT("Client", init_client);
