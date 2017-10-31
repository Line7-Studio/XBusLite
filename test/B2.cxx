#include <XBus.hxx>

#include <gtest/gtest.h>


TEST(B2, PythonEvalStr)
{
    ASSERT_TRUE(XBus::Python::Eval("import os"));
    ASSERT_FALSE(XBus::Python::Eval("import fuck"));
}


TEST(B2, PythonEvalEmbededSourceLoader)
{
    ASSERT_TRUE(XBus::Python::Eval(XBus::EmbededSourceLoader(":EmbededSource")));
    ASSERT_TRUE(XBus::Python::Eval("function()"));
}



int main(int argc, char *argv[])
{
    XBus::ClientHostFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(CLIENT_HOST_PATH);
    XBus::PythonRuntimeFilePath() = XBUS_NATIVE_STRINGIFY_MACRO(PYTHON_RUNTIME_PATH);

    XBus::Python::Initialize(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
