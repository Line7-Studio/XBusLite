#include <XBus.hxx>
#include <XBus.Json.hxx>

#include <gtest/gtest.h>

#include <Windows.h>

class XBusClient
{
public:
    class Job: public XBusLite::Job
    {
    public:
        Job(const XBusLite::Job& job)
        :XBusLite::Job(job)
        {
        }
    public:
        XBus::Json waitForResult() const
        {
            auto result = XBus::Json::parse(XBusLite::Job::waitForSerializedResult());
            // std::cout << result.dump(4) << '\n';

            if( result[0] == false ){
                std::cout << result[1].dump();
                throw std::runtime_error(result[1].dump());
            }
            return result[1];
        }
    };
public:
    XBusClient(const std::string& client_name)
    :m_client_name(client_name)
    {
    }
public:
    Job execute(const std::string& function_name)
    {
        return Job(XBus::Execute(m_client_name, function_name));
    };
public:
    Job execute(const std::string& function_name, \
                const std::string& serialized_function_parameters)
    {
        return Job(XBus::Execute(m_client_name, function_name, \
                                                serialized_function_parameters));
    }
public:
    Job execute(const std::string& function_name, const XBus::Json& parameters)
    {
        return execute(function_name, parameters.dump());
    }
private:
    const std::string m_client_name;
};


TEST(Call, RuturnVoid)
{
    auto client = std::make_unique<XBusClient>("Client");

    {
        auto job = client->execute("fun_void_void");
        ASSERT_EQ(job.waitForResult(), nullptr);
    }
    {
        XBus::Json arguments;
        arguments["value"] = -89;
        auto job = client->execute("fun_void_int", arguments.dump());
        ASSERT_EQ(job.waitForResult(), nullptr);
    }
    {
        XBus::Json arguments;
        arguments["value"] = 3.1415926f;
        auto job = client->execute("fun_void_float", arguments.dump());
        ASSERT_EQ(job.waitForResult(), nullptr);
    }
}


TEST(Call, Echo)
{
    auto client = std::make_unique<XBusClient>("Client");

    {
        auto job = client->execute("fun_echo_void");
        ASSERT_EQ(job.waitForResult(), nullptr);
    }
    {
        XBus::Json arguments;
        arguments["value"] = -89;
        auto job = client->execute("fun_echo_int", arguments.dump());
        ASSERT_EQ(job.waitForResult().get<int>(), -89);
    }
    {
        XBus::Json arguments;
        arguments["value"] = 3.1415926f;
        auto job = client->execute("fun_echo_float", arguments.dump());
        ASSERT_EQ(job.waitForResult().get<float>(), 3.1415926f);
    }
}


int main(int argc, char *argv[])
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::ClientHostFilePath() = L"xbus_client_host\\embed_ffi.exe";
#else // Not On Windows
    XBus::ClientHostFilePath() = "xbus_client_host/embed_ffi";
#endif // XBUS_LITE_PLATFORM_WINDOWS

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::PythonRuntimeFilePath() = L"E:\\Python\\3.6\\x64\\python36.dll";
#endif // XBUS_LITE_PLATFORM_WINDOWS

#ifdef XBUS_LITE_PLATFORM_DARWIN
    XBus::PythonRuntimeFilePath() = "/usr/local/Cellar/python3/3.6.3/Frameworks/Python.framework/Versions/3.6/Python";
#endif // XBUS_LITE_PLATFORM_DARWIN

    ::testing::InitGoogleTest(&argc, argv);

    XBus::CreateClient("Client");
    XBus::WaitClientInitialized("Client");

    return RUN_ALL_TESTS();
}
