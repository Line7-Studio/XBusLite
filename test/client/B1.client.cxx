#include <XBus.hxx>

bool init_client_a(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    return true;
}

bool init_client_b(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    return true;
}

bool init_client_c(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    return true;
}

XBUS_REGISTE_CLIENT("Client/A", init_client_a);
XBUS_REGISTE_CLIENT("Client/B", init_client_b);
XBUS_REGISTE_CLIENT("Client/C", init_client_c);
