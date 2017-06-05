#include <XBus.hxx>
#include <XBus.Json.hxx>

#include <gtest/gtest.h>

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
            return XBus::Json::parse(XBusLite::Job::waitForSerializedResult());
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


TEST(Execution, SyncNoParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        auto job = client->execute("fun_0");
        ASSERT_EQ(job.waitForSerializedResult(), std::string("null"));
    }
    {
        auto job = client->execute("fun_1");
        ASSERT_EQ(job.waitForSerializedResult(), std::string("\"hello xbus\""));
    }
}


int main(int argc, char *argv[])
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::ClientHostFilePath() = L"xbus_client_host\\embed_module.exe";
    XBus::PythonRuntimeFilePath() = L"E:\\Python\\3.6\\x64\\python36.dll";
    // XBus::PythonRuntimeFilePath() = L"E:\\Python\\3.6\\x32\\python36.dll";
    // XBus::PythonRuntimeFilePath() = L"E:\\Python\\3.4\\x32\\python34.dll";
#else // Not On Windows
    XBus::ClientHostFilePath() = "xbus_client_host/embed_module";
    XBus::PythonRuntimeFilePath() = "/usr/local/Cellar/python3/3.6.1/Frameworks/Python.framework/Versions/3.6/Python";
#endif // XBUS_LITE_PLATFORM_WINDOWS
    ::testing::InitGoogleTest(&argc, argv);

    XBus::CreateClient("Client");
    XBus::WaitClientInitialized("Client");

    return RUN_ALL_TESTS();
}
