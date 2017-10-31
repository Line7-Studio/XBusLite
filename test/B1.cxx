#include <XBus.hxx>

#include <gtest/gtest.h>


TEST(B1, CreatClientOne)
{
    XBus::CreateClient("Client/A");

    std::cout << "is client Client/A init done? "
              << XBus::IsClientInitialized("Client/A") << '\n';

    XBus::WaitClientInitialized("Client/A");
    ASSERT_TRUE(XBus::IsClientInitialized("Client/A"));

    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST(B1, CreateClientMulti)
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

TEST(B1, CreateClientWithRename)
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
    XBus::ClientHostFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(CLIENT_HOST_PATH);
    XBus::PythonRuntimeFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(PYTHON_RUNTIME_PATH);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
