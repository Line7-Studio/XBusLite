#include <XBus.hxx>

auto client_python_source = R"(

embed_module = __load_embeded_module__(':EmbededSource')

fun = embed_module.fun

)";

bool init_client_a(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    XBus::Python::Eval(client_python_source);

    return true;

}

XBUS_REGISTE_CLIENT("Client/A", init_client_a);


bool init_client_b(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    XBus::Python::Eval(XBus::EmbededSourceLoader(":EmbededSource"));

    return true;

}

XBUS_REGISTE_CLIENT("Client/B", init_client_b);
