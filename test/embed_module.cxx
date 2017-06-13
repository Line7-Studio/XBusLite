#include <XBus.hxx>

#include <gtest/gtest.h>


TEST(EmbedModule, EvalStr)
{
    auto job = XBus::Execute("Client/A", "fun");
    ASSERT_EQ(job.waitForSerializedResult(), std::string("[true, null]"));
}


TEST(EmbedModule, EvalEmbededSourceLoader)
{
    auto job = XBus::Execute("Client/B", "fun");
    ASSERT_EQ(job.waitForSerializedResult(), std::string("[true, null]"));
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

    XBus::CreateClient("Client/A");
    XBus::WaitClientInitialized("Client/A");

    XBus::CreateClient("Client/B");
    XBus::WaitClientInitialized("Client/B");

    return RUN_ALL_TESTS();
}
