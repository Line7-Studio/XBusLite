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

TEST(Execution, SyncWithParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        std::string parameters = XBus::Json().dump();
        auto job = client->execute("fun_2", parameters);
        ASSERT_EQ(job.waitForSerializedResult(), parameters);
    }
    {
        std::string parameters = XBus::Json("hello world").dump();
        auto job = client->execute("fun_2", parameters);
        ASSERT_EQ(job.waitForSerializedResult(), parameters);
    }
}

TEST(Execution, AsyncNoParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        std::vector<XBusClient::Job> job_list;
        for(int idx = 0; idx < 1000 ; ++idx )
        {
            auto job = client->execute("fun_0");
            job_list.push_back(job);
        }
        for(auto& job : job_list)
        {
            ASSERT_EQ(job.waitForSerializedResult(), std::string("null"));
        }
    }
    {
        std::vector<XBusClient::Job> job_list;
        for(int idx = 0; idx < 1000 ; ++idx )
        {
            auto job = client->execute("fun_1");
            job_list.push_back(job);
        }
        for(auto& job : job_list)
        {
            ASSERT_EQ(job.waitForSerializedResult(), std::string("\"hello xbus\""));
        }
    }
}

TEST(Execution, AsyncWithParameters)
{
    auto client = std::make_unique<XBusClient>("Client");
    {
        std::vector<XBusClient::Job> job_list;
        for(int idx = 0; idx < 10000 ; ++idx )
        {
            XBus::Json parameters(idx);
            if(idx%2!=0)
            {
                auto job = client->execute("fun_2", parameters);
                job_list.push_back(job);
            }
            else
            {
                auto job = client->execute("fun_3", parameters);
                job_list.push_back(job);
            }
        }
        for(int idx = 0; idx < 10000 ; ++idx )
        {
            const auto& job = job_list[idx];
            if(idx%2!=0)
            {
                ASSERT_EQ(job.waitForResult().get<int>(), idx);
            }
            else
            {
                ASSERT_EQ(job.waitForResult().get<int>(), idx*2);
            }
        }
    }
}

int main(int argc, char *argv[])
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::ClientHostFilePath() = L"xbus_client_host\\task_execution.exe";
    XBus::PythonRuntimeFilePath() = L"E:\\Python3\\x64\\python35.dll";
#else // Not On Windows
    XBus::ClientHostFilePath() = "xbus_client_host/task_execution";
    XBus::PythonRuntimeFilePath() = "/usr/local/Cellar/python3/3.6.0/Frameworks/Python.framework/Versions/3.6/Python";
#endif // XBUS_LITE_PLATFORM_WINDOWS

    ::testing::InitGoogleTest(&argc, argv);

    XBus::CreateClient("Client");
    XBus::WaitClientInitialized("Client");

    return RUN_ALL_TESTS();
}
