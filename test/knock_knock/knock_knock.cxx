#include <XBus.hxx>

bool init_client(std::map<std::string, std::string>& /*args*/)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);
    return true;
}

XBUS_REGISTE_CLIENT("Client", init_client);
