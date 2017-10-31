#include <XBus.hxx>

#include <gtest/gtest.h>


TEST(S1, Knock)
{
    XBus::CreateClient("Client", "Speed");
    XBus::WaitClientInitialized("Speed");

    for(size_t idx = 1; idx < 1000000; ++idx)
    {
        auto res = XBus::Knock("Speed");
        ASSERT_EQ(idx, res.id());
    }
}

int main(int argc, char *argv[])
{
    XBus::ClientHostFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(CLIENT_HOST_PATH);
    XBus::PythonRuntimeFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(PYTHON_RUNTIME_PATH);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
