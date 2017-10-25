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


TEST(Execution, SyncNoParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        auto job = client->execute("Return_None");
        ASSERT_EQ(job.waitForResult(), nullptr);
    }
    {
        auto job = client->execute("Return_Text");
        ASSERT_EQ(job.waitForResult().get<std::string>(), std::string("hello xbus"));
    }
    {
        auto job = client->execute("Return_Number_I");
        ASSERT_EQ(job.waitForResult().get<int>(), 1);
    }
    {
        auto job = client->execute("Return_Number_F");
        ASSERT_EQ(job.waitForResult().get<double>(), 3.14);
    }
    {
        auto job = client->execute("Return_Array");
        auto res = job.waitForResult();
        ASSERT_EQ(res[0].get<int>(), 0);
        ASSERT_EQ(res[1].get<int>(), 1);
        ASSERT_EQ(res[2].get<int>(), 2);
        ASSERT_EQ(res[3].get<std::string>(), std::string("3"));

        ASSERT_EQ(res[4][0].get<int>(), 4);
        ASSERT_EQ(res[4][1].get<int>(), 5);
        ASSERT_EQ(res[4][2].get<float>(), 6.0);

        ASSERT_EQ(res[5].get<int>(), -7);
    }
}

TEST(Execution, SyncWithParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        XBus::Json arguments;
        arguments["arg_1"] = -89;
        auto job = client->execute("Echo_One", arguments.dump());
        ASSERT_EQ(job.waitForResult().get<int>(), -89);
    }
    {
        std::string str("Hello World");
        XBus::Json arguments;
        arguments["arg_1"] = str;
        auto job = client->execute("Echo_One", arguments.dump());
        ASSERT_EQ(job.waitForResult().get<std::string>(), str);
    }
    {
        XBus::Json arguments;
        arguments["arg_1"] = -89;
        arguments["arg_2"] = 189;
        auto job = client->execute("Echo_Two_Swap", arguments.dump());
        auto result = job.waitForResult();
        ASSERT_EQ(result[1].get<int>(), -89);
        ASSERT_EQ(result[0].get<int>(), 189);
    }
    {
        std::string str("Hello World");
        XBus::Json arguments;
        arguments["arg_1"] = str;
        arguments["arg_2"] = 3.1415926;
        auto job = client->execute("Echo_Two_Swap", arguments.dump());
        auto result = job.waitForResult();
        ASSERT_EQ(result[1].get<std::string>(), str);
        ASSERT_EQ(result[0].get<double>(), 3.1415926);
    }
}

TEST(Execution, AsyncNoParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    std::vector<XBusClient::Job> job_list;
    for(int idx=0; idx < 1000; ++idx)
    {
        {
            auto job = client->execute("Return_None");
            job_list.push_back(job);
        }
        {
            auto job = client->execute("Return_Text");
            job_list.push_back(job);
        }
        {
            auto job = client->execute("Return_Number_I");
            job_list.push_back(job);
        }
        {
            auto job = client->execute("Return_Number_F");
            job_list.push_back(job);
        }
        {
            auto job = client->execute("Return_Array");
            job_list.push_back(job);
        }
    }
    for(int idx=0; idx < 1000; )
    {
        {
            auto job = job_list[idx++];
            ASSERT_EQ(job.waitForResult(), nullptr);
        }
        {
            auto job = job_list[idx++];
            ASSERT_EQ(job.waitForResult().get<std::string>(), std::string("hello xbus"));
        }
        {
            auto job = job_list[idx++];
            ASSERT_EQ(job.waitForResult().get<int>(), 1);
        }
        {
            auto job = job_list[idx++];
            ASSERT_EQ(job.waitForResult().get<double>(), 3.14);
        }
        {
            auto job = job_list[idx++];
            auto res = job.waitForResult();
            ASSERT_EQ(res[0].get<int>(), 0);
            ASSERT_EQ(res[1].get<int>(), 1);
            ASSERT_EQ(res[2].get<int>(), 2);
            ASSERT_EQ(res[3].get<std::string>(), std::string("3"));

            ASSERT_EQ(res[4][0].get<int>(), 4);
            ASSERT_EQ(res[4][1].get<int>(), 5);
            ASSERT_EQ(res[4][2].get<float>(), 6.0);

            ASSERT_EQ(res[5].get<int>(), -7);
        }
    }
}

TEST(Execution, AsyncWithParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    std::vector<XBusClient::Job> job_list;
    for(int idx=0; idx < 1000; ++idx)
    {
        {
            XBus::Json arguments;
            arguments["arg_1"] = -89;
            auto job = client->execute("Echo_One", arguments.dump());
            job_list.push_back(job);
        }
        {
            std::string str("Hello World");
            XBus::Json arguments;
            arguments["arg_1"] = str;
            auto job = client->execute("Echo_One", arguments.dump());
            job_list.push_back(job);
        }
        {
            XBus::Json arguments;
            arguments["arg_1"] = -89;
            arguments["arg_2"] = 189;
            auto job = client->execute("Echo_Two_Swap", arguments.dump());
            job_list.push_back(job);
        }
        {
            std::string str("Hello World");
            XBus::Json arguments;
            arguments["arg_1"] = str;
            arguments["arg_2"] = 3.1415926;
            auto job = client->execute("Echo_Two_Swap", arguments.dump());
            job_list.push_back(job);
        }
    }
    for(int idx=0; idx < 1000; )
    {
        {
            auto job = job_list[idx++];
            ASSERT_EQ(job.waitForResult().get<int>(), -89);
        }
        {
            auto job = job_list[idx++];
            std::string str("Hello World");
            ASSERT_EQ(job.waitForResult().get<std::string>(), str);
        }
        {
            auto job = job_list[idx++];
            auto result = job.waitForResult();
            ASSERT_EQ(result[1].get<int>(), -89);
            ASSERT_EQ(result[0].get<int>(), 189);
        }
        {
            auto job = job_list[idx++];
            auto result = job.waitForResult();
            std::string str("Hello World");
            ASSERT_EQ(result[1].get<std::string>(), str);
            ASSERT_EQ(result[0].get<double>(), 3.1415926);
        }
    }
}

int main(int argc, char *argv[])
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::ClientHostFilePath() = L"xbus_client_host\\task_execution.exe";
#else // Not On Windows
    XBus::ClientHostFilePath() = "xbus_client_host/task_execution";
#endif // XBUS_LITE_PLATFORM_WINDOWS

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto exe = std::wstring(XBUS_NATIVE_STRINGIFY_MACRO(XBUS_PYTHON_EXECUTABLE));
    XBus::PythonRuntimeFilePath() = (exe + L"/../python36.dll");
#else
    XBus::PythonRuntimeFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(XBUS_PYTHON_EXECUTABLE);
#endif // XBUS_LITE_PLATFORM_WINDOWS

    ::testing::InitGoogleTest(&argc, argv);

    XBus::CreateClient("Client");
    XBus::WaitClientInitialized("Client");

    return RUN_ALL_TESTS();
}
