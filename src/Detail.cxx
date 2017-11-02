#include "Detail.hxx"

namespace XBusLite
{


std::map<str_t, process_handle_t>& ClientNameToChildProcessHandle()
{
    static std::map<str_t, process_handle_t> kv;
    return kv;
}

std::map<str_t, client_init_function_t>& ClientNameToInitFunction()
{
    static std::map<str_t, client_init_function_t> kv;
    return kv;
}

std::map<str_t, embedded_c_function_t>& EmbededFunctionNameToFunction()
{
    static std::map<str_t, embedded_c_function_t> kv;
    return kv;
}


}// namespace XBusLite
