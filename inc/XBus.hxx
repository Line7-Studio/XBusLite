#pragma once

#include <map>
#include <vector>

#include <string>

#include <chrono>
#include <thread>
#include <utility>

#include <memory>

#include <stdexcept>
#include <functional>

namespace XBusLite
{

typedef std::string str_t;
typedef std::map<str_t, str_t> client_init_function_arguments_t;
typedef std::function<bool(client_init_function_arguments_t&)> client_init_function_t;

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    typedef std::wstring file_path_t;
#else // Not on Windows
    typedef std::string  file_path_t;
#endif // XBUS_LITE_PLATFORM_WINDOWS

class Response
{
public:
    Response() = delete;
public:
    Response(const str_t& client_name, const uint32_t response_id)
    :m_client_name(client_name)
    ,m_response_id(response_id)
    {
    }
private:
    const str_t m_client_name;
    const uint32_t m_response_id;
public:
    str_t client_name() const {
        return m_client_name;
    }
public:
    uint32_t id() const {
        return m_response_id;
    }
};

class Job: public Response
{
public:
    struct State
    {
        enum Enum: uint32_t
        {
            NOT_START, DONE, FAILED,
        };
    };
public:
    Job() = delete;
public:
    Job(const str_t& client_name, const uint32_t response_id, const str_t& fun_name)
    :Response(client_name, response_id)
    ,m_fun_name(fun_name)
    {
    }
public:
    Job(const Job& job)
    :Response(job.client_name(), job.id())
    ,m_fun_name(job.fun_name())
    {
    }
private:
    const str_t m_fun_name;
public:
    str_t fun_name() const {
        return m_fun_name;
    }
public:
    Job::State::Enum fetchJobResultState() const;
public:
    str_t fetchSerializedResult() const;
public:
    str_t waitForSerializedResult() const;
public:
    template<typename Period>
    str_t waitForSerializedResult(long long rep) const;
private:
    str_t waitForSerializedResult( \
            std::chrono::duration<long long, std::micro> rep) const;
};


/*
 store the registered `embedded_c_function_name`->`embedded_c_function` here
 */

typedef void* embedded_c_function_t;
std::map<str_t, embedded_c_function_t>& EmbededFunctionNameToFunction();

/*
 store the registered `client_name`->`client_init_function` here
 */
std::map<str_t, client_init_function_t>& ClientNameToInitFunction();

/*
  create client with `client_name` mapped init function,
  this new client's name the `client_name`
 */
void CreateClient(const str_t& client_name);

/*
  create client with `client_name` mapped init function,
  then rename the client with `new_client_name`
 */
void CreateClient(const str_t& client_name, \
                  const str_t& new_client_name);

/*
  create client with `client_name` mapped init function,
  with the provided `arguments_kv` for this init function
 */
void CreateClient(const str_t& client_name, \
                  const client_init_function_arguments_t& arguments_kv);

/*
  create client with `client_name` mapped init function,
  with the provided `arguments_kv` for this init function,
  then rename the client with `new_client_name`
 */
void CreateClient(const str_t& client_name, \
                  const str_t& new_client_name, \
                  const client_init_function_arguments_t& arguments_kv);

void TerminateClient(const str_t& client_name);
bool IsClientInitialized(const str_t& client_name);
void WaitClientInitialized(const str_t& client_name);

Response Knock(const str_t& client_name); // knock knock test

Job Execute(const str_t& client_name, const str_t& function_name);

Job Execute(const str_t& client_name, const str_t& function_name, \
                                      const str_t& serialized_function_parameters);

// set&get the client host process file path
file_path_t& ClientHostFilePath();

// set&get the client python runtime library file path
file_path_t& PythonRuntimeFilePath();

namespace Error
{

class NoneRegisteredClient: public std::runtime_error
{
public:
    NoneRegisteredClient(const std::string& for_what)
    :std::runtime_error("None Registered Client: "+for_what)
    {
    }
};

class ExecutionFailed: public std::runtime_error
{
public:
    ExecutionFailed(const std::string& for_what)
    :std::runtime_error("Execution Failed: "+for_what)
    {
    }
};

}//namespace Error

class EmbededSourceLoader
{
public:
    EmbededSourceLoader(const std::string& source_url);
private:
    std::string source_url_;
public:
    const std::string& url() const {
        return source_url_;
    }
};

struct Python
{
    static bool Eval(const char* source);
    static bool Eval(const std::string& source);
    static bool Eval(const EmbededSourceLoader& source_url);

    static bool Initialize(int argc, char* argv[]);
};

}// namespace XBusLite

/*! Export XBus API !*/
namespace XBus
{
    using XBusLite::ClientHostFilePath;
    using XBusLite::PythonRuntimeFilePath;

    using XBusLite::CreateClient;
    using XBusLite::TerminateClient;

    using XBusLite::IsClientInitialized;
    using XBusLite::WaitClientInitialized;

    using XBusLite::Knock;
    using XBusLite::Execute;

    using XBusLite::Python;

    using XBusLite::EmbededSourceLoader;

}// namespace XBus

#define XBUS_COMBINE_XYZ(X,Y,Z) X##Y##Z
#define XBUS_COMBINE_XY(X,Y) X##Y
#define XBUS_COMBINE(X,Y,Z) XBUS_COMBINE_XYZ(X,Y,Z)
#define XBUS_NONE_USED_SYMBOL() XBUS_COMBINE(_,__LINE__,__COUNTER__)

#define XBUS_NATIVE_STRINGIFY_MACRO(a) XBUS_NATIVE_STRINGIFY_STR(a)

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    #define XBUS_NATIVE_STRINGIFY_STR(a) L#a
#else
    #define XBUS_NATIVE_STRINGIFY_STR(a) #a
#endif // XBUS_LITE_PLATFORM_WINDOWS

#define XBUS_REGISTE_CLIENT(name, fun)                                         \
    namespace XBUS_UNIQUE{                                                     \
        auto XBUS_NONE_USED_SYMBOL() =                                         \
            ::XBusLite::ClientNameToInitFunction()[(name)] = fun;              \
    }//namespace XBUS_UNIQUE


#define XBUS_REGISTE_EMBEDDED_CTYPE_FUNTION(name, fun)                         \
    namespace XBUS_UNIQUE{                                                     \
        auto XBUS_NONE_USED_SYMBOL() =                                         \
            ::XBusLite::EmbededFunctionNameToFunction()[(name)] = (void*)fun;  \
    }//namespace XBUS_UNIQUE


