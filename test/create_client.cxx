#include <XBus.hxx>

#include <gtest/gtest.h>


TEST(Create, One)
{
    XBus::CreateClient("Client/A");

    std::cout << "is client Client/A init done? "
              << XBus::IsClientInitialized("Client/A") << '\n';

    XBus::WaitClientInitialized("Client/A");
    ASSERT_TRUE(XBus::IsClientInitialized("Client/A"));

    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(Create, Multi)
{
    XBus::CreateClient("Client/B");
    XBus::CreateClient("Client/C");

    std::cout << "is client Client/B init done? "
              << XBus::IsClientInitialized("Client/B") << '\n';
    std::cout << "is client Client/C init done? "
              << XBus::IsClientInitialized("Client/C") << '\n';

    XBus::WaitClientInitialized("Client/B");
    ASSERT_TRUE(XBus::IsClientInitialized("Client/B"));

    XBus::WaitClientInitialized("Client/C");
    ASSERT_TRUE(XBus::IsClientInitialized("Client/C"));

    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(Create, RenameClient)
{
    XBus::CreateClient("Client/A", "Host/A");
    XBus::CreateClient("Client/B", "Host/B");
    XBus::CreateClient("Client/C", "Host/C");

    std::cout << "is client Host/A init done? "
              << XBus::IsClientInitialized("Host/A") << '\n';
    std::cout << "is client Host/B init done? "
              << XBus::IsClientInitialized("Host/B") << '\n';
    std::cout << "is client Host/C init done? "
              << XBus::IsClientInitialized("Host/C") << '\n';

    XBus::WaitClientInitialized("Host/A");
    ASSERT_TRUE(XBus::IsClientInitialized("Host/A"));

    XBus::WaitClientInitialized("Host/B");
    ASSERT_TRUE(XBus::IsClientInitialized("Host/B"));

    XBus::WaitClientInitialized("Host/C");
    ASSERT_TRUE(XBus::IsClientInitialized("Host/C"));

    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char *argv[])
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    XBus::ClientHostFilePath() = L"xbus_client_host\\create_client.exe";
    // XBus::PythonRuntimeFilePath() = L"E:\\Python3\\x64\\python35.dll";
    // XBus::PythonRuntimeFilePath() = L"E:\\Python3\\x32\\python35.dll";
    XBus::PythonRuntimeFilePath() = L"C:\\Python34\\python34.dll";
#else // Not On Windows
    XBus::ClientHostFilePath() = "xbus_client_host/create_client";
    XBus::PythonRuntimeFilePath() = "/usr/local/Cellar/python3/3.6.0/Frameworks/Python.framework/Versions/3.6/Python";
#endif // XBUS_LITE_PLATFORM_WINDOWS

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
