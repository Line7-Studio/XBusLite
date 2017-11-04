#include "Detail.hxx"

namespace XBusLite
{

file_path_t check_and_formal_file_path(const file_path_t& file_path);

file_path_t get_file_name(file_path_t file_full_path);
file_path_t get_file_located_dir(file_path_t file_full_path);

file_path_t get_this_executable_file_name();
file_path_t get_this_executable_located_dir();
file_path_t get_this_executable_file_full_path();


namespace {
size_t SourceCodeItemCount;
std::vector<size_t> SourceCodeNameBlockBufferSize;
std::vector<const char*> SourceCodeNameBlockBuffer;
std::vector<size_t> SourceCodeDataBlockBufferSize;
std::vector<const char*> SourceCodeDataBlockBuffer;
} // namespace

EmbededSourceLoader::EmbededSourceLoader(const std::string& source_url)
:source_url_(source_url)
{
}

void EmbededSourceInitialize(const unsigned char* resource_struct,
                             const unsigned char* name_buffer,
                             const unsigned char* data_buffer)
{
    const auto resource_struct_buffer = (size_t*)(resource_struct);
    auto name_size = *(resource_struct_buffer + 0);
    auto data_size = *(resource_struct_buffer + 1);

    SourceCodeNameBlockBufferSize.push_back(name_size);
    SourceCodeNameBlockBuffer.push_back((char*)name_buffer);

    SourceCodeDataBlockBufferSize.push_back(data_size);
    SourceCodeDataBlockBuffer.push_back((char*)data_buffer);

    SourceCodeItemCount += 1;
}

void EmbededSourceFinalize(const unsigned char* resource_struct,
                           const unsigned char* resource_name,
                           const unsigned char* resource_data)
{

}

class DynamicLibraryLoader
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    HMODULE m_handle;
public:
    DynamicLibraryLoader(const file_path_t& library_file_path)
    :m_handle(::LoadLibrary(library_file_path.c_str()))
    {
        if( m_handle == NULL ){
            // TODO: should NOT be std::runtime_error
            wprintf(L"load python shared library failed! %s\n", library_file_path.c_str());
            throw std::runtime_error("load python shared library failed!\n");
        }
    }
    template<typename T>
    auto Load(const char* fun_name)
    {
        return reinterpret_cast<T>(::GetProcAddress(m_handle, fun_name));
    }
#else // Not On Windows
    void* m_handle;
public:
    DynamicLibraryLoader(const file_path_t& library_file_path)
    :m_handle(::dlopen(library_file_path.c_str(), RTLD_LAZY))
    {
        if( m_handle == NULL ){
            // TODO: should NOT be std::runtime_error
            printf("load python shared library failed! %s\n", library_file_path.c_str());
            throw std::runtime_error("load python shared library failed!\n");
        }
    }
    template<typename T>
    auto Load(const char* fun_name)
    {
        return reinterpret_cast<T>(::dlsym(m_handle, fun_name));
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS
};

namespace PYTHON
{
    typedef void PyObject;
    typedef char32_t Py_UCS4;
    typedef ptrdiff_t Py_ssize_t;
    typedef PyObject* (*PyCFunction)(PyObject* self, PyObject* args);

    struct PyMethodDef {
        const char  *ml_name = nullptr; /* The name of the built-in function/method */
        PyCFunction ml_meth;            /* The C function that implements it */
        int         ml_flags;           /* Combination of METH_xxx flags, which mostly
                                           describe the args expected by the C func */
        const char  *ml_doc;            /* The __doc__ attribute, or NULL */
    };

    typedef struct PyMethodDef PyMethodDef;

    int (*PyRun_SimpleString)(const char* command);

    void (*PyErr_Clear)();
    void (*PyErr_Print)();

    PyObject* (*PyLong_FromVoidPtr)(void* p);

    int (*PyObject_Print)(PyObject* o, FILE* fp, int flags);
    PyObject* (*PyObject_CallObject)(PyObject* callable_object, PyObject* args);

    int (*PyObject_HasAttrString)(PyObject* o, const char* attr_name);
    int (*PyObject_SetAttrString)(PyObject* o, const char* attr_name, PyObject* v);
    PyObject* (*PyObject_GetAttrString)(PyObject* o, const char* attr_name);

    PyObject* (*PyTuple_New)(Py_ssize_t len);
    int (*PyTuple_SetItem)(PyObject* p, Py_ssize_t pos, PyObject* o);

    PyObject* (*PyUnicode_FromStringAndSize)(const char* u, Py_ssize_t size);
    char* (*PyUnicode_AsUTF8AndSize)(PyObject* unicode, Py_ssize_t* size);
    Py_ssize_t (*PyUnicode_GetLength)(PyObject* unicode);
    Py_UCS4* (*PyUnicode_AsUCS4Copy)(PyObject* unicode);

    PyObject* (*PyMarshal_ReadObjectFromString)(const char*, Py_ssize_t);

    PyObject* (*PyEval_GetBuiltins)();

    PyObject* (*PyEval_EvalCode)(PyObject*, PyObject*, PyObject*);

    PyObject* (*PyDict_GetItemString)(PyObject*, const char*);
    int (*PyDict_SetItemString)(PyObject* p, const char* key, PyObject* val);

    PyObject* (*PyImport_AddModule)(const char*);
    PyObject* (*PyImport_AddModuleObject)(PyObject* name);
    PyObject* (*PyImport_ExecCodeModule)(const char* name, PyObject* co);
    PyObject* (*PyImport_ExecCodeModuleObject)( \
            PyObject* name, PyObject* co, PyObject* pathname, PyObject* cpathname);

    PyObject* (*PyModule_GetDict)(PyObject*);
    int (*PyModule_AddFunctions)(PyObject* module, PyMethodDef* functions);
    int (*PyModule_AddStringConstant)(PyObject* module, const char* name, const char* value);

    wchar_t* (*Py_DecodeLocale)(const char* arg, size_t* size);

    void (*PySys_SetArgvEx)(int argc, wchar_t** argv, int updatepath);

    void (*PyMem_Free)(void* p);

    void (*Py_IncRef)(PyObject*);
    void (*Py_DecRef)(PyObject*);

    PyObject* MODULE_MAIN_DICT;
    PyObject* MODULE_XBUS_DICT;

    PyObject* OBJECT_NONE;
    PyObject* FUNCTION_EXEC;
}// PYTHON

namespace /**/
{

using namespace PYTHON;

PyObject* LoadEmbededModule(PyObject* self, PyObject* arg)
{
    auto size = PyUnicode_GetLength(arg);
    auto buffer = PyUnicode_AsUCS4Copy(arg);

#ifdef _MSC_VER
    std::wstring_convert< std::codecvt_utf8<int32_t>, int32_t > conv;
    auto name = conv.to_bytes(\
        reinterpret_cast<const int32_t*>(std::u32string(buffer, size).c_str()));
#else
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    auto name = conv.to_bytes(std::u32string(buffer, size));
#endif // _MSC_VER

    PyMem_Free(buffer);

    size_t source_code_index = 0;
    for ( ; source_code_index < SourceCodeItemCount; ++source_code_index)
    {
        auto name_buffer = SourceCodeNameBlockBuffer[source_code_index];
        auto name_size = SourceCodeNameBlockBufferSize[source_code_index];
        if( (name_size == name.size()) && \
            (name.compare(0, name_size, name_buffer) == 0) )
        {
            break;
        }
    }

    if( source_code_index == SourceCodeItemCount ){
        printf("LoadEmbededModule Try Load None Exists Module %s\n", name.c_str());
        return PYTHON::OBJECT_NONE;
    }

    auto code_object = PyMarshal_ReadObjectFromString( \
                            SourceCodeDataBlockBuffer[source_code_index],
                            SourceCodeDataBlockBufferSize[source_code_index]
                        );

    auto module_object = PyImport_ExecCodeModuleObject(arg, code_object, arg, NULL);

    Py_DecRef(code_object);

    return module_object;
}

PyObject* FetchEmbeddedFunctionAddress(PyObject* self, PyObject* arg)
{
    auto size = PyUnicode_GetLength(arg);
    auto buffer = PyUnicode_AsUCS4Copy(arg);

#ifdef _MSC_VER
    std::wstring_convert< std::codecvt_utf8<int32_t>, int32_t > conv;
    auto name = conv.to_bytes(\
        reinterpret_cast<const int32_t*>(std::u32string(buffer, size).c_str()));
#else
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    auto name = conv.to_bytes(std::u32string(buffer, size));
#endif // _MSC_VER

    PyMem_Free(buffer);

    const auto& functions = EmbededFunctionNameToFunction();

    auto itr = functions.find(name);
    if(itr == functions.end()){
        printf("FetchEmbeddedFunctionAddress Try Fetch None Exists Function %s\n", name.c_str());
        return OBJECT_NONE;
    }

    // return OBJECT_NONE;
    return PyLong_FromVoidPtr(itr->second);
}

}// end namespace

auto Py_Object_Call(void* callable_object, void* args) {
    auto res = PYTHON::PyObject_CallObject(callable_object, args);
    if( res == NULL ){
        throw std::runtime_error("::PyObject_CallObject Failed");
    }
    return res;
}

auto Py_Tuple_New(size_t len) {
    auto res = PYTHON::PyTuple_New(len);
    if( res == NULL ){
        throw std::runtime_error("::PyTuple_New Failed");
    }
    return res;
}

void Py_Tuple_Set_Item(void* p, size_t pos, void* o) {
    if( PYTHON::PyTuple_SetItem(p, pos, o) ){
        throw std::runtime_error("::PyTuple_SetItem Failed");
    }
}

auto Py_Unicode_From_UTF8_Str(const std::string& str) {
    auto res = PYTHON::PyUnicode_FromStringAndSize(str.data(), str.size());
    if( res == NULL ){
        throw std::runtime_error("::PyUnicode_FromStringAndSize Failed");
    }
    return res;
}

auto Py_Unicode_To_UTF8_Str(void* unicode) {
    ptrdiff_t size;
    auto buffer = PYTHON::PyUnicode_AsUTF8AndSize(unicode, &size);
    if( buffer == NULL ){
        throw std::runtime_error("::PyUnicode_AsUTF8AndSize Failed");
    }
    return std::string(buffer, size);
}

void Py_Ref_Dec(void* object) {
    if( object != NULL ){
        PYTHON::Py_DecRef(object);
    }
}

// Export To XBusLite For Public API
bool Python::Eval(const char* source)
{
    auto result = PYTHON::PyRun_SimpleString(source);
    if( result == 0 ){
        return true;
    }
    PYTHON::PyErr_Print();
    return false;
}

// Export To XBusLite For Public API
bool Python::Eval(const std::string& source)
{
    auto result = PYTHON::PyRun_SimpleString(source.c_str());
    if( result == 0 ){
        return true;
    }
    PYTHON::PyErr_Print();
    return false;
}

// Export To XBusLite For Public API
bool Python::Eval(const EmbededSourceLoader& source_loader)
{
    auto source_url = source_loader.url();

    size_t source_code_index = 0;
    for ( ; source_code_index < SourceCodeItemCount; ++source_code_index)
    {
        auto name_buffer = SourceCodeNameBlockBuffer[source_code_index];
        auto name_size = SourceCodeNameBlockBufferSize[source_code_index];
        if( source_url.compare(0, name_size, name_buffer) == 0 ){
            break;
        }
    }
    if( source_code_index == SourceCodeItemCount ){
        // TODO:: should NOT be std::runtime_error
        printf("Python::Eval Try to Call None Exists Code");
        throw std::runtime_error("Python::Eval Try to Call None Exists Code");
    }

    auto code_object = PYTHON::PyMarshal_ReadObjectFromString(
            SourceCodeDataBlockBuffer[source_code_index],
            SourceCodeDataBlockBufferSize[source_code_index]
        );

    auto result_object = PYTHON::PyEval_EvalCode( code_object, \
                    PYTHON::MODULE_MAIN_DICT, PYTHON::MODULE_MAIN_DICT );

    PYTHON::Py_DecRef(code_object);

    if(result_object == NULL){
        PYTHON::PyErr_Print();
        return false;
    }

    PYTHON::Py_DecRef(result_object);
    return true;
}

// Export To XBusLite For Public API
bool Python::Initialize(int argc, char* argv[])
{
    // printf("%s %s\n", __FILE__, __FUNCTION__);

    auto PyLy = std::make_unique<DynamicLibraryLoader>(PythonRuntimeFilePath());

    {   namespace PF = PYTHON;

        #define X_LOAD_PY_FUN_PTR(X) \
            PF::X = PyLy->Load<decltype(PF::X)>(#X);

        X_LOAD_PY_FUN_PTR(PyImport_AddModule);
        X_LOAD_PY_FUN_PTR(PyImport_AddModuleObject);
        X_LOAD_PY_FUN_PTR(PyImport_ExecCodeModule);
        X_LOAD_PY_FUN_PTR(PyImport_ExecCodeModuleObject);

        X_LOAD_PY_FUN_PTR(PyModule_AddStringConstant);
        X_LOAD_PY_FUN_PTR(PyModule_AddFunctions);
        X_LOAD_PY_FUN_PTR(PyModule_GetDict);

        X_LOAD_PY_FUN_PTR(PyObject_HasAttrString);
        X_LOAD_PY_FUN_PTR(PyObject_GetAttrString);
        X_LOAD_PY_FUN_PTR(PyObject_CallObject);
        X_LOAD_PY_FUN_PTR(PyObject_Print);

        X_LOAD_PY_FUN_PTR(PyLong_FromVoidPtr);

        X_LOAD_PY_FUN_PTR(PyTuple_New);
        X_LOAD_PY_FUN_PTR(PyTuple_SetItem);

        X_LOAD_PY_FUN_PTR(PyUnicode_AsUCS4Copy);
        X_LOAD_PY_FUN_PTR(PyUnicode_AsUTF8AndSize);
        X_LOAD_PY_FUN_PTR(PyUnicode_FromStringAndSize);
        X_LOAD_PY_FUN_PTR(PyUnicode_GetLength);

        X_LOAD_PY_FUN_PTR(PyDict_GetItemString);
        X_LOAD_PY_FUN_PTR(PyDict_SetItemString);

        X_LOAD_PY_FUN_PTR(PyMarshal_ReadObjectFromString);

        X_LOAD_PY_FUN_PTR(PyEval_GetBuiltins);
        X_LOAD_PY_FUN_PTR(PyEval_EvalCode);

        X_LOAD_PY_FUN_PTR(PyRun_SimpleString);

        X_LOAD_PY_FUN_PTR(PySys_SetArgvEx);

        X_LOAD_PY_FUN_PTR(Py_DecodeLocale);

        X_LOAD_PY_FUN_PTR(PyMem_Free);

        X_LOAD_PY_FUN_PTR(PyErr_Clear);
        X_LOAD_PY_FUN_PTR(PyErr_Print);

        X_LOAD_PY_FUN_PTR(Py_DecRef);
        X_LOAD_PY_FUN_PTR(Py_IncRef);

        #undef X_LOAD_PY_FUN_PTR
    } // end load python functions

    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        // fix python lib path on Windows
        auto python_dll_path = check_and_formal_file_path(PythonRuntimeFilePath());
        auto python_dll_dir = get_file_located_dir(python_dll_path);
        auto python_dll_file_name = get_file_name(python_dll_path); // without dir

        auto python_zip_file = python_dll_file_name.substr(0, \
                                    python_dll_file_name.rfind(L".")) + L".zip";

        auto python_lib_path = python_dll_dir + L"\\" + python_zip_file + L";"
                             + python_dll_dir + L"\\Lib;"
                             + python_dll_dir + L"\\Lib\\site-packages;"
                             + python_dll_dir + L"\\DLLs;";

        PyLy->Load<void(*)(const wchar_t*)>("Py_SetPath")(python_lib_path.c_str());

    #else // NOT XBUS_LITE_PLATFORM_WINDOWS

        // fix python lib path for *nix
        auto python_bin_path = check_and_formal_file_path(PythonRuntimeFilePath());
        auto python_bin_dir = get_file_located_dir(python_bin_path);
        auto python_lib_dir = check_and_formal_file_path(python_bin_dir+"/../lib");

        auto python_lib_path =
           python_lib_dir + "/" + XBUS_PYTHON_ZIP_PACKAGE_NAME + ":"
         + python_lib_dir + "/" + XBUS_PYTHON_LIB_VERSION_PREFIX + ":"
         + python_lib_dir + "/" + XBUS_PYTHON_LIB_VERSION_PREFIX + "/lib-dynload:"
         + python_lib_dir + "/" + XBUS_PYTHON_LIB_VERSION_PREFIX + "/site-packages:"
         + "";

        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        PyLy->Load<void(*)(const wchar_t*)>("Py_SetPath")( \
                        convert.from_bytes(python_lib_path).c_str());

    #endif // XBUS_LITE_PLATFORM_WINDOWS

    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        ::SetEnvironmentVariable(L"PYTHONIOENCODING", L"UTF-8");
    #else // Not On Windows
        ::setenv("PYTHONIOENCODING", "UTF-8", 0);
    #endif // XBUS_LITE_PLATFORM_WINDOWS

    // init python
    PyLy->Load<void(*)(void)>("Py_Initialize")();

    { /******************/ using namespace PYTHON; /*******************/
    { /************************* fix sys.argv *************************/
        auto wargv = new wchar_t*[argc];
        for (int idx = 0; idx < argc; ++idx) {
            auto size = std::strlen(argv[idx]);
            wargv[idx] = Py_DecodeLocale(argv[idx], &size);
        }

        PySys_SetArgvEx(argc, wargv, 0); // set sys.argv
        for (int idx = 0; idx < argc; ++idx) {
            PyMem_Free(wargv[idx]);
        }
        delete[] wargv;
    }
    { /********************** patch __builtins__ ********************/
        auto module = PyImport_AddModule("builtins");
        auto dict = PyObject_GetAttrString(module, "__dict__");

        // Fetch None from builtins
        OBJECT_NONE = PyDict_GetItemString(dict, "None");

        auto exe_dir = get_this_executable_located_dir();

    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        auto exe_dir_str = Py_Unicode_From_UTF8_Str(convert.to_bytes(exe_dir).c_str());
        PyDict_SetItemString(dict, "__executable_located_folder__", exe_dir_str);
    #else  // Not On Windows
        auto exe_dir_str = Py_Unicode_From_UTF8_Str(exe_dir.c_str());
        PyDict_SetItemString(dict, "__executable_located_folder__", exe_dir_str);
    #endif// XBUS_LITE_PLATFORM_WINDOWS

        // static PyMethodDef methods_list[2];
        static PyMethodDef methods_list[3];

        methods_list[0].ml_name  = "__load_embeded_module__";
        methods_list[0].ml_meth  = LoadEmbededModule;
        methods_list[0].ml_flags = 0x0008; // METH_O
        methods_list[0].ml_doc   = NULL;

        methods_list[1].ml_name  = "__fetch_embedded_function_address__";
        methods_list[1].ml_meth  = FetchEmbeddedFunctionAddress;
        methods_list[1].ml_flags = 0x0008; // METH_O
        methods_list[1].ml_doc   = NULL;

        PyModule_AddFunctions(module, methods_list);
    }
    { /**********************     __main__      ********************/
        auto module_main = PyImport_AddModule("__main__");
        MODULE_MAIN_DICT = PyModule_GetDict(module_main);
        Py_IncRef(MODULE_MAIN_DICT);
    }
    } /****************** using namespace PYTHON; ******************/

    return true;
}


#ifdef XBUS_SOURCE_FOR_CLIENT_HOST


void* PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_NO_PARAMETERS_FUNCTION;
void* PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_WITH_PARAMETERS_FUNCTION;

static const char* XBUS_MODULE_TXT = R"(

import xbus
import json as xbus_json

xbus.declare = type('xbus.declare', (object, ), {})()

xbus.__declared_functions_no_arguments__ = {}
xbus.__declared_functions_has_arguments__ = {}


def function(real_function):
    from inspect import signature
    fs_para = signature(real_function).parameters.values()

    if len(fs_para) == 0:
        xbus.__declared_functions_no_arguments__[real_function.__name__] = real_function
    else:
        xbus.__declared_functions_has_arguments__[real_function.__name__] = real_function
    return real_function

xbus.declare.function = function


def function(fun_name: str):
    try:
        result = xbus.__declared_functions_no_arguments__[fun_name]()
    except Exception as error:
        print("XBus Wrappred Function Execute Error:")
        print("%s"%fun_name, "Error: \"%s\""%str(error))
        result_info_list = [False, error]
    else:
        result_info_list = [True, result]
    finally:
        return xbus_json.dumps(result_info_list)

xbus.__execute_json_serialized_no_parameters_function__ = function


def function(fun_name: str, fun_para: str):
    try:
        result = xbus.__declared_functions_has_arguments__[fun_name](**xbus_json.loads(fun_para))
    except Exception as error:
        print("XBus Wrappred Function Execute Error:")
        print("%s"%fun_name, "Error: \"%s\""%str(error))
        result_info_list = [False, error]
    else:
        result_info_list = [True, result]
    finally:
        return xbus_json.dumps(result_info_list)

xbus.__execute_json_serialized_with_parameters_function__ = function

del function

)";

// do the dynamical load
bool InitPythonRuntime(int argc, char* argv[])
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    Python::Initialize(argc, argv);

    {   using namespace PYTHON;

    // add moduel xbus for next runing XBUS_MODULE_TXT
    auto module_xbus = PyImport_AddModule("xbus");
    Py_IncRef(module_xbus);

    MODULE_XBUS_DICT = PyModule_GetDict(module_xbus);

    // in XBUS_MODULE_TXT we defined xbus module function
    PyRun_SimpleString(XBUS_MODULE_TXT);

    // funtion for : __execute_json_serialized_no_parameters_function__
    PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_NO_PARAMETERS_FUNCTION = \
                    PyDict_GetItemString( MODULE_XBUS_DICT, \
                        "__execute_json_serialized_no_parameters_function__"
                );
    // funtion for : __execute_json_serialized_with_parameters_function__
    PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_WITH_PARAMETERS_FUNCTION = \
                    PyDict_GetItemString( MODULE_XBUS_DICT, \
                        "__execute_json_serialized_with_parameters_function__"
                );
    } // end using namespace PYTHON

    return true;
}

std::pair<Job::State::Enum, std::vector<char>>
    ExecuteJob(const std::vector<char>& fun_name)
{
    auto arguments = Py_Tuple_New(1);
    auto function_name = Py_Unicode_From_UTF8_Str( \
                            std::string(fun_name.data(), fun_name.size()) );

    Py_Tuple_Set_Item(arguments, 0, function_name);

    auto result = Py_Object_Call( \
        PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_NO_PARAMETERS_FUNCTION, arguments);

    auto result_data = Py_Unicode_To_UTF8_Str(result);

    std::vector<char> serialized_result(result_data.begin(), result_data.end());

    Py_Ref_Dec(arguments);
    Py_Ref_Dec(result);

    return std::make_pair(Job::State::DONE, serialized_result);
}

std::pair<Job::State::Enum, std::vector<char>>
    ExecuteJob(const std::vector<char>& fun_name, const std::vector<char>& fun_args)
{
    auto arguments = Py_Tuple_New(2);
    auto function_name = Py_Unicode_From_UTF8_Str( \
                            std::string(fun_name.data(), fun_name.size()) );
    auto function_args = Py_Unicode_From_UTF8_Str( \
                            std::string(fun_args.data(), fun_args.size()) );

    Py_Tuple_Set_Item(arguments, 0, function_name);
    Py_Tuple_Set_Item(arguments, 1, function_args);

    auto result = Py_Object_Call( \
        PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_WITH_PARAMETERS_FUNCTION, arguments);

    auto result_data = Py_Unicode_To_UTF8_Str(result);

    std::vector<char> serialized_result(result_data.begin(), result_data.end());

    Py_Ref_Dec(arguments);
    Py_Ref_Dec(result);

    return std::make_pair(Job::State::DONE, serialized_result);
}

#endif // XBUS_SOURCE_FOR_CLIENT_HOST


}// namespace XBusLite
