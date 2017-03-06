#include "XBus.hxx"

#include <queue>
#include <vector>
#include <sstream>
#include <unordered_map>

#include <locale>
#include <codecvt>

#include <random>

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <type_traits>

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    #include <fcntl.h>
    #include <spawn.h>
    #include <signal.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <dlfcn.h>     // for dynamic load python
#endif //XBUS_LITE_PLATFORM_WINDOWS

#ifdef XBUS_LITE_PLATFORM_DARWIN
    #include <sys/types.h> // for kqueue
    #include <sys/event.h> // for kqueue
    #include <sys/time.h>  // for kqueue
    #include <mach-o/dyld.h> // for module path
#endif //XBUS_LITE_PLATFORM_DARWIN


namespace XBusLite
{

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    typedef HANDLE share_mem_handle_t;
    typedef HANDLE process_handle_t;
    typedef HANDLE pipe_handle_t;
#else // Not on Windows
    typedef int share_mem_handle_t;
    typedef pid_t process_handle_t;
    typedef int pipe_handle_t;
#endif // XBUS_LITE_PLATFORM_WINDOWS

std::map<str_t, client_init_function_t>& ClientNameToInitFunction()
{
    static std::map<str_t, client_init_function_t> kv;
    return kv;
}

std::map<str_t, process_handle_t>& ClientNameToChildProcessHandle()
{
    static std::map<str_t, process_handle_t> kv;
    return kv;
}

#ifdef XBUS_LITE_PLATFORM_WINDOWS

HANDLE init_job_session()
{
    auto job_session_object = ::CreateJobObject(NULL, NULL);
    if( job_session_object == NULL )
    {
        throw std::runtime_error(__FUNCTION__" ::CreateJobObject Failed");
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if( 0 == ::SetInformationJobObject(job_session_object, \
                    JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)) )
    {
        throw std::runtime_error(__FUNCTION__" ::SetInformationJobObject Failed");
    }
    return job_session_object;
}

HANDLE JOB_SESSION_OBJECT = init_job_session();

#else // Not On Windows

#endif // XBUS_LITE_PLATFORM_WINDOWS


const file_path_t generate_a_random_file_name(const int random_str_len)
{
    file_path_t random_str(random_str_len, 0);
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    const std::wstring alphanums = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#else // not on Windows
     const std::string alphanums = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif // XBUS_LITE_PLATFORM_WINDOWS
    std::mt19937 rg{std::random_device{}()};
    std::uniform_int_distribution<> pick(0, int(alphanums.size()) - 1);

    for(int idx = 0; idx < random_str_len; ++idx) {
        random_str[idx] = alphanums[pick(rg)];
    }
    return random_str;
}

file_path_t check_and_formal_file_path(const file_path_t& file_path)
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto attr = ::GetFileAttributes(file_path.c_str());
    if( attr == INVALID_FILE_ATTRIBUTES ){
        throw std::runtime_error(__FUNCTION__" ::GetFileAttributes Failed");
    }
    // see the reference from MSDN
    file_path_t fixed_file_path;
    for(uint32_t size = 512; ; size += 512)
    {
        fixed_file_path.resize(size);
        DWORD truncated_size = \
            ::GetFullPathName( file_path.c_str(), size-2, \
                const_cast<wchar_t*>(fixed_file_path.data()), 0 \
            );

        if( truncated_size == 0 ) {
            throw std::runtime_error(__FUNCTION__" ::GetFullPathName Failed");
        }
        if( truncated_size < (size-2) ){
            fixed_file_path.resize(truncated_size);
            break;
        }
    }
    return fixed_file_path;
#else // Not On Windows
    file_path_t fixed_file_path(PATH_MAX, 0);
    auto fixed_file_path_ptr = \
                        ::realpath( \
                            const_cast<char*>(file_path.c_str()), \
                            const_cast<char*>(fixed_file_path.c_str()) \
                        );

    if(fixed_file_path_ptr == nullptr)
    {
        printf("%s realpath error: %s\n", __FUNCTION__, strerror(errno));
        throw std::runtime_error("::realpath failed");
    }
    // TODO: need formal??
    return file_path_t(fixed_file_path_ptr);
#endif // XBUS_LITE_PLATFORM_WINDOWS
}

file_path_t get_file_name(file_path_t file_full_path)
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto found_pos = file_full_path.rfind(L"\\");
#else // Not On Windows
    auto found_pos = file_full_path.rfind("/");
#endif //XBUS_LITE_PLATFORM_WINDOWS

    return file_full_path.substr(found_pos+1);
}

file_path_t get_file_located_dir(file_path_t file_full_path)
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto found_pos = file_full_path.rfind(L"\\");
#else // Not On Windows
    auto found_pos = file_full_path.rfind("/");
#endif //XBUS_LITE_PLATFORM_WINDOWS

    return file_full_path.substr(0, found_pos);
}

file_path_t get_this_executable_file_full_path()
{
    file_path_t result;

#ifdef XBUS_LITE_PLATFORM_DARWIN
    file_path_t app_full_path;
    // see the reference from Apple
    for(uint32_t size = 512; ; size += 512)
    {
        app_full_path.resize(size);
        uint32_t buffer_size = size;
        auto res = _NSGetExecutablePath( \
                        const_cast<char*>(app_full_path.data()), &buffer_size);

        if( res == -1 ){
            app_full_path.resize(buffer_size);
            continue;
        }
        if( res == 0 ){
            break;
        }
    }

    auto abs_path_buffer = ::realpath(app_full_path.c_str(), NULL);
    result = std::string(abs_path_buffer);
    ::free(abs_path_buffer);
#endif //XBUS_LITE_PLATFORM_DARWIN

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    file_path_t app_full_path;
    // see the reference from MSDN
    for(uint32_t size = 512; ; size += 512)
    {
        app_full_path.resize(size);
        DWORD truncated_size = ::GetModuleFileName(0, \
            const_cast<wchar_t*>(app_full_path.data()), size-2);

        if( truncated_size == 0 ) {
            throw std::runtime_error("::GetModuleFileName Failed");
        }
        if( truncated_size < size - 2 ){
            app_full_path.resize(truncated_size);
            break;
        }
    }

    result = file_path_t(app_full_path);
#endif //XBUS_LITE_PLATFORM_WINDOWS

    return result;
}

file_path_t get_this_executable_file_name()
{
    return get_file_name(get_this_executable_file_full_path());
}

file_path_t get_this_executable_located_dir()
{
    return get_file_located_dir(get_this_executable_file_full_path());
}

enum RequestType : uint32_t
{
    KNOCK,
    CREATE_JOB_NO_PARAMETERS,
    CREATE_JOB_WITH_PARAMETERS,
    FETCH_JOB_STATE,
    FETCH_JOB_RESULT,
};

/*
  memory layout:
      first size(size_t) bytes store the size information, last bytes store
      the data
  class usage:
      ServerSideSharedMem must call first, then the ClientSideSharedMem
      init with the randome name from ServerSideSharedMem->name();
 */
class IFixedSizeSharedMem
{
protected:
    char* m_buffer = nullptr;
    share_mem_handle_t m_handle = 0;
public:
    char* data() const
    {
        return m_buffer + sizeof(size_t);
    }
protected:
    file_path_t m_name;
public:
    decltype(m_name) name() const
    {
        return m_name;
    }
};

class ServerSideSharedMem: public IFixedSizeSharedMem
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
public:
    ServerSideSharedMem(size_t size)
    {
        // using random object name to avoid confilicte
        m_name = generate_a_random_file_name(16);
        DWORD required_size = static_cast<DWORD>(sizeof(size_t) + size);

        m_handle = ::CreateFileMapping( \
                             INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, \
                             0, required_size, m_name.c_str());

        if( m_handle == NULL ){
            throw std::runtime_error(__FUNCTION__" ::CreateFileMapping Failed");
        }

        m_buffer = (char*)::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if( m_buffer == NULL ){
            ::CloseHandle(m_handle);
            throw std::runtime_error(__FUNCTION__" ::MapViewOfFile Failed");
        }

        // store the size info
        *((size_t*)m_buffer) = required_size;
    }
public:
    ~ServerSideSharedMem()
    {
        if( 0 == ::UnmapViewOfFile(m_buffer) ){
            perror(__FUNCTION__" ::UnmapViewOfFile Failed");
        }
        if( 0 == ::CloseHandle(m_handle) ){
            perror(__FUNCTION__" ::CloseHandle Failed");
        }
    }
#else // Not On Windows
public:
    ServerSideSharedMem(size_t size)
    {
        // Mac OS only support max length 31
        m_name = generate_a_random_file_name(16);
        auto required_size = sizeof(size_t) + size;

        m_handle = ::shm_open(m_name.data(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if( m_handle == -1 ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::shm_open failed");
        }

        if( ::ftruncate(m_handle, required_size) != 0 ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::ftruncate failed");
        }

        m_buffer = (char*)::mmap(0, required_size, PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        if( m_buffer == MAP_FAILED ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::mmap failed");
        }

        // store the size info
        *((size_t*)m_buffer) = required_size;
    }
public:
    ~ServerSideSharedMem()
    {
        auto required_size = *((size_t*)m_buffer);

        if( ::munmap(m_buffer, required_size) ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::munmap failed");
        }
        if( ::shm_unlink(m_name.c_str()) != 0 ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::shm_unlink failed");
        }
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS
};

class ClientSideSharedMem: public IFixedSizeSharedMem
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
public:
    ClientSideSharedMem(const file_path_t& name)
    {
        m_name = name;
        m_handle = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name.data());

        if( m_handle == NULL ){
            throw std::runtime_error(__FUNCTION__" ::OpenFileMapping Failed");
        }

        m_buffer = (char*)::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if( m_buffer == NULL ){
            throw std::runtime_error(__FUNCTION__" ::MapViewOfFile Failed");
        }
    }
public:
    ~ClientSideSharedMem()
    {
        // TODO:
    }
#else // Not On Windows
public:
    ClientSideSharedMem(const file_path_t& name)
    {
        m_name = name;
        m_handle = ::shm_open(name.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);

        if( m_handle == -1 ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::shm_open failed");
        }

        auto buffer = (char*)::mmap(NULL, sizeof(size_t), PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        auto mem_size = *((size_t*)buffer);

        m_buffer = (char*)::mmap(NULL, mem_size, PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        if( m_buffer == MAP_FAILED ){
            throw std::runtime_error(str_t(__FUNCTION__) + " ::mmap failed");
       }
    }
public:
    ~ClientSideSharedMem()
    {
        if( 0 != ::munmap(m_buffer, *((size_t*)m_buffer)) ) {
            printf("%s ::munmap failed\n", __FUNCTION__);
        }
        if( 0 != ::close(m_handle) ){
            printf("%s ::close failed\n", __FUNCTION__);
        }
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS
};


template<typename T>
static T read_pipe_for_value(pipe_handle_t pipe_handle)
{
    using namespace std::placeholders;  // for _1, _2, _3...

    T value = 0;
    auto value_buffer = reinterpret_cast<char*>(&value);

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    DWORD read_size;

    auto read_pipe = std::bind(::ReadFile, pipe_handle, _1, _2, _3, _4);
    if( read_pipe(value_buffer, (DWORD)sizeof(T), &read_size, nullptr) != TRUE )
    {
        printf(__FUNCTION__ "::ReadFile Failed 1\n");
        throw std::runtime_error(__FUNCTION__ "::ReadFile Failed 1\n");
    }
    if( read_size != sizeof(T) )
    {
        printf(__FUNCTION__ "::ReadFile Failed 2\n");
        throw std::runtime_error(__FUNCTION__ "::ReadFile Failed 2\n");
    }
#else // Not On Windows
    auto read_pipe = std::bind(::read, pipe_handle, _1, _2);
    if( read_pipe(value_buffer, sizeof(T)) != sizeof(T) )
    {
        printf("%s ::read failed 1\n", __FUNCTION__);
        throw std::runtime_error(str_t(__FUNCTION__)+" ::read 1");
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS

    return value;
}

template<typename T>
static void write_pipe_for_value(pipe_handle_t pipe_handle, const T value)
{
    using namespace std::placeholders;  // for _1, _2, _3...

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto write_pipe = std::bind(::WriteFile, pipe_handle, _1, _2, _3, _4);

    DWORD write_size;
    if( write_pipe(&value, (DWORD)sizeof(T), &write_size, nullptr) != TRUE )
    {
        printf(__FUNCTION__ "::WriteFile Failed 1\n");
        throw std::runtime_error(__FUNCTION__ "::WriteFile Failed 1\n");
    }
    if( write_size != sizeof(T) )
    {
        printf(__FUNCTION__ "::WriteFile Failed 2\n");
        throw std::runtime_error(__FUNCTION__ "::WriteFile Failed 2\n");
    };
#else // Not On Windows
    auto write_pipe = std::bind(::write, pipe_handle, _1, _2);

    if( write_pipe(&value, sizeof(T)) != sizeof(T) )
    {
        printf("%s ::write failed 1\n", __FUNCTION__);
        throw std::runtime_error(str_t(__FUNCTION__)+" ::write failed 1");
    };
#endif // XBUS_LITE_PLATFORM_WINDOWS
}

template<typename T>
static T read_pipe_for_buffer(pipe_handle_t pipe_handle)
{
    using namespace std::placeholders;  // for _1, _2, _3...

    typedef typename T::value_type buffer_value_t;
    typedef typename T::pointer buffer_pointer_t;

    const auto buffer_size = read_pipe_for_value<size_t>(pipe_handle);

    T buffer_data(buffer_size/sizeof(buffer_value_t), 0);
    if( buffer_size == 0){
        return buffer_data;
    }

    auto data_buffer = reinterpret_cast<char*>( \
                        const_cast<buffer_pointer_t>(buffer_data.data()));

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto read_pipe = std::bind(::ReadFile, pipe_handle, _1, _2, _3, _4);

    DWORD read_size;
    if( read_pipe(data_buffer, (DWORD)buffer_size, &read_size, nullptr) != TRUE )
    {
        printf(__FUNCTION__ "::ReadFile Failed 1\n");
        throw std::runtime_error(__FUNCTION__ "::ReadFile Failed 1\n");
    }
    if( read_size != buffer_size )
    {
        printf(__FUNCTION__ "::ReadFile Failed 2\n");
        throw std::runtime_error(__FUNCTION__ "::ReadFile Failed 2\n");
    }
#else // Not On Windows
    auto read_pipe = std::bind(::read, pipe_handle, _1, _2);

    if( read_pipe(data_buffer, buffer_size) != buffer_size )
    {
        printf("%s ::read failed 2\n", __FUNCTION__);
        throw std::runtime_error(str_t(__FUNCTION__)+" ::read failed 2");
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS

    return buffer_data;
}

template<typename T>
static void write_pipe_for_buffer(pipe_handle_t pipe_handle, const T& buffer)
{
    using namespace std::placeholders;  // for _1, _2, _3...

    typedef typename T::value_type buffer_value_t;
    typedef typename T::pointer buffer_pointer_t;

    const auto buffer_size = buffer.size() * sizeof(buffer_value_t);
    write_pipe_for_value<size_t>(pipe_handle, buffer_size);

    if( buffer_size == 0 ){
        return;
    }

    auto data_buffer = reinterpret_cast<char*>( \
                            const_cast<buffer_pointer_t>(buffer.data()));

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto write_pipe = std::bind(::WriteFile, pipe_handle, _1, _2, _3, _4);

    DWORD write_size;
    if( write_pipe(data_buffer, (DWORD)buffer_size, &write_size, nullptr) != TRUE )
    {
        printf(__FUNCTION__ "::WriteFile Failed 1\n");
        throw std::runtime_error(__FUNCTION__ "::WriteFile Failed 1\n");
    }
    if( write_size != buffer_size )
    {
        printf(__FUNCTION__ "::WriteFile Failed 2\n");
        throw std::runtime_error(__FUNCTION__ "::WriteFile Failed 2\n");
    };
#else // Not On Windows
    auto write_pipe = std::bind(::write, pipe_handle, _1, _2);

    if( write_pipe(data_buffer, buffer_size) != buffer_size )
    {
        printf("%s ::write failed 2\n", __FUNCTION__);
        throw std::runtime_error(str_t(__FUNCTION__)+" ::write failed 2");
    };
#endif // XBUS_LITE_PLATFORM_WINDOWS
}

class CommunicationTube
{
private:
    struct TubePack
    {
        pipe_handle_t for_read;
        pipe_handle_t for_write;

        template<class T>
        typename std::enable_if<std::is_fundamental<T>::value>::type
            write(const T value)
        {
            write_pipe_for_value<T>(for_write, value);
        }

        template<class T>
        typename std::enable_if<!std::is_fundamental<T>::value>::type
            write(const T& data)
        {
            write_pipe_for_buffer<T>(for_write, data);
        }

        template<class T>
        typename std::enable_if<std::is_fundamental<T>::value, T>::type
            read()
        {
            return read_pipe_for_value<T>(for_read);
        }

        template<class T>
        typename std::enable_if<!std::is_fundamental<T>::value, T>::type
            read()
        {
            return read_pipe_for_buffer<T>(for_read);
        }
    };
public:
    TubePack request;
    TubePack response;
public:
    static void create_communication_tube(TubePack* tube)
    {
#ifdef XBUS_LITE_PLATFORM_WINDOWS
        SECURITY_ATTRIBUTES security_attributes;
        security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        security_attributes.lpSecurityDescriptor = NULL;
        security_attributes.bInheritHandle = TRUE;

        const size_t max_buf_size = 4*1024*1024; // 4 MB
        HANDLE pipe_handle_read, pipe_handle_write;
        if( 0 == ::CreatePipe(&pipe_handle_read, &pipe_handle_write, \
                                    &security_attributes, max_buf_size) )
        {
            throw std::runtime_error(__FUNCTION__" ::CreatePipe Failed");
        }
        tube->for_read = pipe_handle_read;
        tube->for_write = pipe_handle_write;
#else // Not On Windows
        int pipi_handle_fildes[2];
        if( ::pipe(pipi_handle_fildes) != 0) {
            throw std::runtime_error(str_t(__FUNCTION__)+" ::pipe failed");
        }
        tube->for_read = pipi_handle_fildes[0];
        tube->for_write = pipi_handle_fildes[1];
#endif // XBUS_LITE_PLATFORM_WINDOWS
    };
public:
    CommunicationTube()
    {
        create_communication_tube(&request);
        create_communication_tube(&response);
    }
};

enum ClientInitState: uint32_t
{
    NOT_START,
    BOOTING,
    SUCCESSED,
    FAILED
};

struct SharedInformationBlock
{
    ClientInitState m_client_init_state = ClientInitState::NOT_START;
    CommunicationTube m_commuication_tube;
};

ServerSideSharedMem* ClientNameToServerSideSharedMem(const str_t& client_name)
{
    static std::map<str_t, ServerSideSharedMem*> all_client_server_side_shared_mem;
    if( all_client_server_side_shared_mem[client_name] != nullptr ){
        return all_client_server_side_shared_mem[client_name];
    }
    auto ptr = new ServerSideSharedMem(sizeof(SharedInformationBlock));
    all_client_server_side_shared_mem[client_name] = ptr;
    return ptr;
}

/*
  get CommunicationTube from server side
 */
CommunicationTube* ClientNameToCommunicationTube(const str_t& client_name)
{
    static std::map<str_t, CommunicationTube*> all_client_communication_tube;
    if( all_client_communication_tube[client_name] != nullptr ){
        return all_client_communication_tube[client_name];
    }
    auto buffer = ClientNameToServerSideSharedMem(client_name)->data();
    buffer += sizeof(ClientInitState); // move the right memory address
    new (buffer) CommunicationTube; // init CommunicationTube here
    auto obj_ptr = reinterpret_cast<CommunicationTube*>(buffer);
    all_client_communication_tube[client_name] = obj_ptr;
    return obj_ptr;
}

str_t& ClientName()
{
    static str_t name;
    return name;
}

file_path_t& ClientHostFilePath()
{
    static file_path_t file_path;
    return file_path;
}

file_path_t& PythonRuntimeFilePath()
{
    static file_path_t file_path;
    return file_path;
}

void CreateClient(const str_t& client_name)
{
    return CreateClient(client_name, client_name);
}

void CreateClient(const str_t& client_name, \
                  const str_t& new_client_name)
{
    return CreateClient(client_name, new_client_name, \
                        client_init_function_arguments_t());
}

void CreateClient(const str_t& client_name,\
                  const client_init_function_arguments_t& arguments_kv)
{
    return CreateClient(client_name, client_name, arguments_kv);
}

void CreateClient(const str_t& client_name, \
                  const str_t& new_client_name, \
                  const client_init_function_arguments_t& arguments_kv)
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    if( ClientNameToInitFunction()[client_name] == nullptr ){
        printf("not registered client name\n");
        throw Error::NoneRegisteredClient(client_name);
    }

    auto shared_mem = ClientNameToServerSideSharedMem(new_client_name);
    auto communication_tube = ClientNameToCommunicationTube(new_client_name);

    // then create client process
#ifdef XBUS_LITE_PLATFORM_WINDOWS

    PROCESS_INFORMATION prcInfo;
    STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,             \
                                 (DWORD)CW_USEDEFAULT, (DWORD)CW_USEDEFAULT, \
                                 (DWORD)CW_USEDEFAULT, (DWORD)CW_USEDEFAULT, \
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                \
                               };

    DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
    creation_flags |= CREATE_BREAKAWAY_FROM_JOB;
    // creation_flags |= DETACHED_PROCESS;
    // creation_flags |= CREATE_NEW_CONSOLE;

    // do not forget the space between arguments
    std::wstringstream args;
    args << ::GetCommandLine() << L" ";
    args << L"--shared_mem="<< shared_mem->name() << L" ";

    auto client_host_executable = get_this_executable_located_dir() \
                                        + L"\\"+ ClientHostFilePath();

    // According MSDN, CreateProcess unicode version
    // here we maybe have a bug with const_cast
    auto success = ::CreateProcess( client_host_executable.c_str(), \
                                    const_cast<wchar_t*>(args.str().c_str()), \
                                    NULL, NULL, TRUE, creation_flags, 0, NULL, \
                                    &startupInfo, &prcInfo );

    if( success == 0 ){
        printf("CreateProcess Failed %lu\n", ::GetLastError());
        throw std::runtime_error(__FUNCTION__" ::CreateProcess Failed");
    }

    auto process_handle = prcInfo.hProcess;

    if( 0 == ::AssignProcessToJobObject(JOB_SESSION_OBJECT, process_handle) )
    {
        printf("AssignProcessToJobObject Failed %lu\n", ::GetLastError());
        throw std::runtime_error(__FUNCTION__" ::AssignProcessToJobObject Failed");
    }

    // don't forget to close this handle
    ::CloseHandle(prcInfo.hThread);

#else // Not on Windows
    pid_t process_handle;

    auto client_host_executable = get_this_executable_located_dir() \
                                            + "/" + ClientHostFilePath();

    auto args_shared_mem = str_t("--shared_mem=") + shared_mem->name();

    char* spawned_args[3] = {0};
    spawned_args[0] = const_cast<char*>(client_host_executable.c_str());
    spawned_args[1] = const_cast<char*>(args_shared_mem.c_str());

    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);

    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->request.for_read);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->request.for_write);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->response.for_read);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->response.for_write);

    // posix_spawn_file_actions_addopen(&file_actions, \
    //                         STDOUT_FILENO, "/dev/null", O_WRONLY, 0);

    // posix_spawn_file_actions_addopen(&file_actions, \
    //                         STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    auto success = ::posix_spawn(&process_handle, \
                        client_host_executable.c_str(), \
                        &file_actions, nullptr, spawned_args, nullptr);

    posix_spawn_file_actions_destroy(&file_actions);

    if( success != 0 ){
        throw std::runtime_error(str_t(__FUNCTION__)+" ::posix_spawn failed");
    }

#endif // XBUS_LITE_PLATFORM_WINDOWS

    communication_tube->request.write<str_t>(client_name);
    communication_tube->request.write<file_path_t>(PythonRuntimeFilePath());

    communication_tube->request.write<size_t>(arguments_kv.size());
    for(auto&& kv: arguments_kv)
    {
        communication_tube->request.write<decltype(kv.first)>(kv.first);
        communication_tube->request.write<decltype(kv.second)>(kv.second);
    }

    ClientNameToChildProcessHandle()[new_client_name] = process_handle;
}

bool IsClientInitialized(const str_t& client_name)
{
    auto shared_mem = ClientNameToServerSideSharedMem(client_name);

    auto state = reinterpret_cast<ClientInitState*>(shared_mem->data());

    switch( *state )
    {
        case ClientInitState::NOT_START:
        case ClientInitState::BOOTING:
            return false;
        break;
        case ClientInitState::SUCCESSED:
            return true;
        break;
        case ClientInitState::FAILED:
            throw std::runtime_error("Client Init Failed");
        break;
    }
    return false;
}

void WaitClientInitialized(const str_t& client_name)
{
    while( IsClientInitialized(client_name) != true )
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

#ifdef XBUS_SOURCE_FOR_CLIENT_HOST

/*
 store the CommunicationTube from client side
 */
static CommunicationTube* ClientCommunicationTube = nullptr;

class DynamicLibraryLoader
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    HMODULE m_handle;
public:
    DynamicLibraryLoader(const file_path_t& library_file_path)
    :m_handle(::LoadLibrary(library_file_path.c_str()))
    {
        if( m_handle == NULL ){
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

void* PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_NO_PARAMETERS_FUNCTION;
void* PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_WITH_PARAMETERS_FUNCTION;

static const char* XBUS_MODULE_TXT = R"(

import xbus
import json as xbus_json

xbus.declare = type('xbus.declare', (object, ), {})()

xbus.__declared_functions__ = {}

def function(real_function):
    def xbus_function_wrapper(*args, **kwargs):
        return real_function(*args, **kwargs)
    xbus.__declared_functions__[real_function.__name__] = xbus_function_wrapper
    return real_function

xbus.declare.function = function

def function(fun_name: str):
    return xbus_json.dumps(xbus.__declared_functions__[fun_name]())

xbus.__execute_json_serialized_no_parameters_function__ = function

def function(fun_name: str, fun_para: str):
    return xbus_json.dumps( \
             xbus.__declared_functions__[fun_name](xbus_json.loads(fun_para)))

xbus.__execute_json_serialized_with_parameters_function__ = function

del function

)";


namespace PYTHON_FUNTIONS
{
    typedef void PyObject;
    typedef ptrdiff_t Py_ssize_t;

    int (*PyRun_SimpleString)(const char *command);

    PyObject* (*PyObject_CallObject)(PyObject *callable_object, PyObject *args);

    PyObject* (*PyTuple_New)(Py_ssize_t len);
    int (*PyTuple_SetItem)(PyObject *p, Py_ssize_t pos, PyObject *o);

    PyObject* (*PyUnicode_FromStringAndSize)(const char *u, Py_ssize_t size);
    char* (*PyUnicode_AsUTF8AndSize)(PyObject *unicode, Py_ssize_t *size);

    void (*Py_DecRef)(PyObject *o);

}// PYTHON_FUNTIONS

void Py_Run_Txt(const char* txt) {
    if( 0 != PYTHON_FUNTIONS::PyRun_SimpleString(txt) ){
        throw std::runtime_error("::PyRun_SimpleString Failed");
    }
}

auto Py_Object_Call(void* callable_object, void* args) {
    auto res = PYTHON_FUNTIONS::PyObject_CallObject(callable_object, args);
    if( res == NULL ){
        throw std::runtime_error("::PyObject_CallObject Failed");
    }
    return res;
}

auto Py_Tuple_New(size_t len) {
    auto res = PYTHON_FUNTIONS::PyTuple_New(len);
    if( res == NULL ){
        throw std::runtime_error("::PyTuple_New Failed");
    }
    return res;
}

void Py_Tuple_Set_Item(void* p, size_t pos, void* o) {
    if( PYTHON_FUNTIONS::PyTuple_SetItem(p, pos, o) ){
        throw std::runtime_error("::PyTuple_SetItem Failed");
    }
}

auto Py_Unicode_From_UTF8_Str(const std::string& str) {
    auto res = PYTHON_FUNTIONS::PyUnicode_FromStringAndSize(str.data(), str.size());
    if( res == NULL ){
        throw std::runtime_error("::PyUnicode_FromStringAndSize Failed");
    }
    return res;
}

auto Py_Unicode_To_UTF8_Str(void* unicode) {
    ptrdiff_t size;
    auto buffer = PYTHON_FUNTIONS::PyUnicode_AsUTF8AndSize(unicode, &size);
    if( buffer == NULL ){
        throw std::runtime_error("::PyUnicode_AsUTF8AndSize Failed");
    }
    return std::string(buffer, size);
}

void Py_Ref_Dec(void* object) {
    if( object != NULL ){
        PYTHON_FUNTIONS::Py_DecRef(object);
    }
}

// Export To XBusLite For Public API
int Python::Eval(const char* source)
{
    return PYTHON_FUNTIONS::PyRun_SimpleString(source);
}

// do the dynamical load
bool InitPythonRuntime(int argc, char const *argv[])
{
    auto PyLy = std::make_unique<DynamicLibraryLoader>(PythonRuntimeFilePath());

    {   namespace PF = PYTHON_FUNTIONS;

        #define X_LOAD_PY_FUN_PTR(X) \
            PF::X = PyLy->Load<decltype(PF::X)>(#X);

        X_LOAD_PY_FUN_PTR(PyRun_SimpleString);
        X_LOAD_PY_FUN_PTR(PyObject_CallObject);
        X_LOAD_PY_FUN_PTR(PyTuple_New);
        X_LOAD_PY_FUN_PTR(PyTuple_SetItem);
        X_LOAD_PY_FUN_PTR(PyUnicode_FromStringAndSize);
        X_LOAD_PY_FUN_PTR(PyUnicode_AsUTF8AndSize);
        X_LOAD_PY_FUN_PTR(Py_DecRef);

        #undef X_LOAD_PY_FUN_PTR
    } // end load

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    auto python_dll_path = check_and_formal_file_path(PythonRuntimeFilePath());
    auto python_dll_dir = get_file_located_dir(python_dll_path);
    auto python_dll_file_name = get_file_name(python_dll_path); // without dir

    auto python_zip_file = python_dll_file_name.substr(0, \
                                python_dll_file_name.rfind(L".")) + L".zip";

    auto python_lib_path = python_dll_dir + L"\\" + python_zip_file + L";"
                         + python_dll_dir + L"\\Lib;"
                         + python_dll_dir + L"\\DLLs;";

    // fix python lib path on Windows
    PyLy->Load<void(*)(const wchar_t*)>("Py_SetPath")(python_lib_path.c_str());
#endif // XBUS_LITE_PLATFORM_WINDOWS

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    ::SetEnvironmentVariable(L"PYTHONIOENCODING", L"UTF-8");
#else // Not On Windows
    ::setenv("PYTHONIOENCODING", "UTF-8", 0);
#endif // XBUS_LITE_PLATFORM_WINDOWS

    // init python
    PyLy->Load<void(*)(void)>("Py_Initialize")();

    // add moduel xbus for next runing XBUS_MODULE_TXT
    auto module_xbus_dict = PyLy->Load<void*(*)(void*)>("PyModule_GetDict")( \
                    PyLy->Load<void*(*)(const char*)>("PyImport_AddModule")("xbus")
                );

    // in XBUS_MODULE_TXT we defined xbus module function
    PyLy->Load<int(*)(const char*)>("PyRun_SimpleString")(XBUS_MODULE_TXT);

    // get and store the xbus funtions
    auto py_get_dict_item = \
            PyLy->Load<void*(*)(void*,const char*)>("PyDict_GetItemString");

    // funtion for : __execute_json_serialized_no_parameters_function__
    PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_NO_PARAMETERS_FUNCTION = \
                    py_get_dict_item( module_xbus_dict, \
                        "__execute_json_serialized_no_parameters_function__"
                );
    // funtion for : __execute_json_serialized_with_parameters_function__
    PY_FUN_PTR_EXECUTE_JSON_SERIALIZED_WITH_PARAMETERS_FUNCTION = \
                    py_get_dict_item( module_xbus_dict, \
                        "__execute_json_serialized_with_parameters_function__"
                );

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

//-----------------------------------------------------------------------------
enum class JobFuntionType: uint32_t { NO_PARAMETERS, WITH_PARAMETERS, };
//-----------------------------------------------------------------------------
void execution_job_queue_push(uint32_t id);
uint32_t wait_and_pop_execution_job_queue();
//-----------------------------------------------------------------------------
JobFuntionType mutex_fetch_job_fun_type(const uint32_t id);
void mutex_store_job_arguments(const uint32_t id, \
                                    const std::vector<char>& fun_name);
void mutex_store_job_arguments(const uint32_t id, \
                                    const std::vector<char>& fun_name,
                                        const std::vector<char>& fun_args);
void mutex_fetch_then_erase_job_arguments(const uint32_t id, \
                                             std::vector<char>* fun_name);
void mutex_fetch_then_erase_job_arguments(const uint32_t id, \
                                            std::vector<char>* fun_name,
                                                std::vector<char>* fun_args);
//-----------------------------------------------------------------------------
uint32_t mutex_fetch_job_state(const uint32_t id);
void mutex_store_job_state(const uint32_t id, const uint32_t state);
//-----------------------------------------------------------------------------
std::vector<char> mutex_fetch_then_erase_job_result(const uint32_t id);
void mutex_store_job_result(const uint32_t id, const std::vector<char>& result);
//-----------------------------------------------------------------------------

void listen_server_request_in_child_thread()
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    const auto tube = ClientCommunicationTube;
    std::atomic<uint32_t> response_id_counter{0};

    while(true)
    {
        auto request_type = tube->request.read<uint32_t>();

        switch(request_type)
        {
            case RequestType::KNOCK:
            {
                response_id_counter++;
                uint32_t response_id = response_id_counter;
                tube->response.write<uint32_t>(response_id);
            }
            break;
            case RequestType::FETCH_JOB_STATE:
            {
                auto job_id = tube->request.read<uint32_t>();
                auto job_state = mutex_fetch_job_state(job_id);
                tube->response.write<uint32_t>(job_state);
            }
            break;
            case RequestType::FETCH_JOB_RESULT:
            {
                auto job_id = tube->request.read<uint32_t>();
                auto job_result = mutex_fetch_then_erase_job_result(job_id);
                tube->response.write<decltype(job_result)>(job_result);
            }
            break;
            case RequestType::CREATE_JOB_NO_PARAMETERS:
            {
                response_id_counter++;
                uint32_t this_new_job_id = response_id_counter;

                auto fun_name = tube->request.read<std::vector<char>>();
                mutex_store_job_arguments(this_new_job_id, fun_name);
                mutex_store_job_state(this_new_job_id, Job::State::NOT_START);

                tube->response.write<uint32_t>(this_new_job_id);
                execution_job_queue_push(this_new_job_id);
            }
            break;
            case RequestType::CREATE_JOB_WITH_PARAMETERS:
            {
                response_id_counter++;
                uint32_t this_new_job_id = response_id_counter;

                auto fun_name = tube->request.read<std::vector<char>>();
                auto fun_args = tube->request.read<std::vector<char>>();

                mutex_store_job_arguments(this_new_job_id, fun_name, fun_args);
                mutex_store_job_state(this_new_job_id, Job::State::NOT_START);

                tube->response.write<uint32_t>(this_new_job_id);
                execution_job_queue_push(this_new_job_id);
            }
            default:
            {
            #ifdef XBUS_DEVELOP_MODE
                throw std::runtime_error("Listen Server Request Get Unknown Type");
            #endif// XBUS_DEVELOP_MODE
            }
            break;
        };
    }
}

void execute_server_request_in_child_main_thread()
{
    while(true)
    {
        uint32_t current_job_id = wait_and_pop_execution_job_queue();
        switch(mutex_fetch_job_fun_type(current_job_id))
        {
            case JobFuntionType::NO_PARAMETERS:
            {
                std::vector<char> fun_name;
                mutex_fetch_then_erase_job_arguments(current_job_id, &fun_name);
                auto result = ExecuteJob(fun_name);
                mutex_store_job_result(current_job_id, result.second);
                mutex_store_job_state(current_job_id, result.first);
            }
            break;
            case JobFuntionType::WITH_PARAMETERS:
            {
                std::vector<char> fun_name, fun_args;
                mutex_fetch_then_erase_job_arguments(current_job_id, &fun_name,
                                                                     &fun_args);
                auto result = ExecuteJob(fun_name, fun_args);
                mutex_store_job_result(current_job_id, result.second);
                mutex_store_job_state(current_job_id, result.first);
            }
            break;
        }
    }
}

// -----------------------------------------------------------------------------

std::queue<uint32_t> execution_job_queue;
std::mutex execution_job_condition_mutex;
std::condition_variable execution_job_condition_variable;

void execution_job_queue_push(uint32_t this_new_job_id)
{
    {
    std::unique_lock<std::mutex> lk(execution_job_condition_mutex);
    execution_job_queue.push(this_new_job_id);
    }

    execution_job_condition_variable.notify_one();
}

uint32_t wait_and_pop_execution_job_queue()
{
    std::unique_lock<std::mutex> lk(execution_job_condition_mutex);
    while( execution_job_queue.empty() ){
        execution_job_condition_variable.wait(lk);
    }

    auto current_job_id = execution_job_queue.front();
    execution_job_queue.pop();
    return current_job_id;
}

// -----------------------------------------------------------------------------

std::mutex execution_job_arguments_mutex;
std::unordered_map<uint32_t, JobFuntionType> execution_job_fun_type_cache;
std::unordered_map<uint32_t, std::vector<char>> execution_job_fun_name_cache;
std::unordered_map<uint32_t, std::vector<char>> execution_job_fun_args_cache;

JobFuntionType mutex_fetch_job_fun_type(const uint32_t id)
{
    std::unique_lock<std::mutex> lk(execution_job_arguments_mutex);
    return execution_job_fun_type_cache[id];
}
void mutex_fetch_then_erase_job_arguments(const uint32_t id,
                                        std::vector<char>* fun_name)
{
    std::unique_lock<std::mutex> lk(execution_job_arguments_mutex);
    std::swap(*fun_name, execution_job_fun_name_cache[id]);
    execution_job_fun_name_cache.erase(id);
    execution_job_fun_type_cache.erase(id);
}
void mutex_fetch_then_erase_job_arguments(const uint32_t id,
                                        std::vector<char>* fun_name,
                                        std::vector<char>* fun_args)
{
    std::unique_lock<std::mutex> lk(execution_job_arguments_mutex);
    std::swap(*fun_name, execution_job_fun_name_cache[id]);
    std::swap(*fun_args, execution_job_fun_args_cache[id]);
    execution_job_fun_args_cache.erase(id);
    execution_job_fun_name_cache.erase(id);
    execution_job_fun_type_cache.erase(id);
}
void mutex_store_job_arguments(const uint32_t id, const std::vector<char>& fun_name)
{
    std::unique_lock<std::mutex> lk(execution_job_arguments_mutex);
    execution_job_fun_type_cache[id] = JobFuntionType::NO_PARAMETERS;
    execution_job_fun_name_cache[id] = fun_name;
}
void mutex_store_job_arguments(const uint32_t id, const std::vector<char>& fun_name,
                                                  const std::vector<char>& fun_args)
{
    std::unique_lock<std::mutex> lk(execution_job_arguments_mutex);
    execution_job_fun_type_cache[id] = JobFuntionType::WITH_PARAMETERS;
    execution_job_fun_name_cache[id] = fun_name;
    execution_job_fun_args_cache[id] = fun_args;
}
// -----------------------------------------------------------------------------
std::mutex execution_job_state_mutex;
std::unordered_map<uint32_t, uint32_t> execution_job_state_cache;

std::mutex execution_job_result_mutex;
std::unordered_map<uint32_t, std::vector<char>> execution_job_result_cache;

uint32_t mutex_fetch_job_state(const uint32_t id)
{
    std::unique_lock<std::mutex> lk(execution_job_state_mutex);
    return execution_job_state_cache[id];
}
void mutex_store_job_state(const uint32_t id, const uint32_t state)
{
    std::unique_lock<std::mutex> lk(execution_job_state_mutex);
    execution_job_state_cache[id] = state;
}
std::vector<char> mutex_fetch_then_erase_job_result(const uint32_t id)
{
    {
        std::unique_lock<std::mutex> lk(execution_job_state_mutex);
        execution_job_state_cache.erase(id);
    }

    std::unique_lock<std::mutex> lk(execution_job_result_mutex);
    auto result = execution_job_result_cache[id];
    execution_job_result_cache.erase(id);
    return result;
}
void mutex_store_job_result(const uint32_t id, const std::vector<char>& result)
{
    std::unique_lock<std::mutex> lk(execution_job_result_mutex);
    execution_job_result_cache[id] = result;
}
// -----------------------------------------------------------------------------

#ifdef XBUS_LITE_PLATFORM_DARWIN
// kill the client when server process terminate
void* child_thread_handle_server_termination(void* /*arguments*/)
{
    int kq = kqueue();
    struct kevent kev;
    EV_SET(&kev, getppid(), EVFILT_PROC, EV_ADD|EV_ENABLE, NOTE_EXIT, 0, NULL);

    (void) kevent(kq, &kev, 1, &kev, 1, NULL);
    exit(EXIT_SUCCESS);

    return nullptr;
}
#endif // XBUS_LITE_PLATFORM_DARWIN

int ClientHostMain(int argc, char const *argv[])
{
    printf("%s %s\n", __FILE__, __FUNCTION__);

    // get shared_mem name from argv
    str_t fetched_shared_mem_file_name;
    for(int idx = 0; idx < argc; ++idx)
    {
        if( strncmp(argv[idx], "--shared_mem", 12) == 0 )
        {
            fetched_shared_mem_file_name = str_t(argv[idx]+13);
        }
    }

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    auto shared_mem_name = convert.from_bytes(fetched_shared_mem_file_name);
#else  // Not On Windows
    auto shared_mem_name = fetched_shared_mem_file_name;
    // create thread listen server process termination
    pthread_t thread;
    if(::pthread_create(&thread, 0, &child_thread_handle_server_termination, 0)){
        throw std::runtime_error(str_t(__FUNCTION__)+" ::pthread_create failed");
    }
#endif // NOT XBUS_LITE_PLATFORM_WINDOWS

    auto shared_mem = std::make_unique<ClientSideSharedMem>(shared_mem_name);

    auto buffer = shared_mem->data();
    auto client_init_state = reinterpret_cast<ClientInitState*>(buffer);
    buffer += sizeof(ClientInitState);
    // keep this in global
    ClientCommunicationTube = reinterpret_cast<CommunicationTube*>(buffer);

    // get boot information
    ClientName() = ClientCommunicationTube->request.read<str_t>();
    // store the python runtime file path
    PythonRuntimeFilePath() = ClientCommunicationTube->request.read<file_path_t>();

    client_init_function_arguments_t arguments_kv;

    auto arguments_kv_count = ClientCommunicationTube->request.read<size_t>();
    for(size_t idx = 0; idx < arguments_kv_count; ++idx)
    {
        auto k = ClientCommunicationTube->request.read<str_t>();
        auto v = ClientCommunicationTube->request.read<str_t>();
        arguments_kv[k] = v;
    }

    // init python host here
    auto init_res = InitPythonRuntime(argc, argv);
    if( init_res != true  ){
        (*client_init_state) = ClientInitState::FAILED;
    } else {
        (*client_init_state) = ClientInitState::SUCCESSED;
    }

    // execute init function
    ClientNameToInitFunction()[ClientName()](arguments_kv);

    // init successed
    *(client_init_state) = ClientInitState::SUCCESSED;

    // create new thread to listen server request
    new std::thread(listen_server_request_in_child_thread);

    // start run loop to execute server request
    execute_server_request_in_child_main_thread();

    return 0;
}

#endif // XBUS_SOURCE_FOR_CLIENT_HOST


Response Knock(const str_t& client_name)
{
    auto tube = ClientNameToCommunicationTube(client_name);

    tube->request.write<uint32_t>(RequestType::KNOCK);
    auto response_id = tube->response.read<uint32_t>();

    return Response(client_name, response_id);
}

Job Execute(const str_t& client_name, const str_t& function_name)
{
    auto tube = ClientNameToCommunicationTube(client_name);

    tube->request.write<uint32_t>(RequestType::CREATE_JOB_NO_PARAMETERS);
    tube->request.write<str_t>(function_name);
    auto this_new_job_id = tube->response.read<uint32_t>();

    return Job(client_name, this_new_job_id, function_name);
}

Job Execute(const str_t& client_name, const str_t& function_name, \
                                      const str_t& serialized_function_parameters)
{
    auto tube = ClientNameToCommunicationTube(client_name);

    tube->request.write<uint32_t>(RequestType::CREATE_JOB_WITH_PARAMETERS);
    tube->request.write<str_t>(function_name);
    tube->request.write<str_t>(serialized_function_parameters);
    auto this_new_job_id = tube->response.read<uint32_t>();

    return Job(client_name, this_new_job_id, function_name);
}

Job::State::Enum Job::fetchJobResultState() const
{
    auto tube = ClientNameToCommunicationTube(client_name());

    tube->request.write<uint32_t>(RequestType::FETCH_JOB_STATE);
    tube->request.write<uint32_t>(id());

    auto state = tube->response.read<uint32_t>();

    return Job::State::Enum(state);
}

str_t Job::fetchSerializedResult() const
{
    auto tube = ClientNameToCommunicationTube(client_name());

    tube->request.write<uint32_t>(RequestType::FETCH_JOB_RESULT);
    tube->request.write<uint32_t>(id());

    auto result = tube->response.read<str_t>();

    return result;
}

str_t Job::waitForSerializedResult( \
        std::chrono::duration<long long, std::micro> rep) const
{
    while(true)
    {
        auto state = fetchJobResultState();
        bool should_wait_continue = false;
        switch( state )
        {
            case Job::State::NOT_START:
                should_wait_continue = true;
            break;
            case Job::State::DONE:
                should_wait_continue = false;
            break;
            case Job::State::FAILED:
                throw Error::ExecutionFailed(client_name() + " " + fun_name());
            break;
        }
        if( should_wait_continue != true ){
            break;
        } else {
            // hack for msvc bug
        #ifdef XBUS_LITE_PLATFORM_WINDOWS
            ::SwitchToThread();
        #else
            std::this_thread::sleep_for(rep);
        #endif //XBUS_LITE_PLATFORM_WINDOWS
        }
    }
    return fetchSerializedResult();
}

template<>
str_t Job::waitForSerializedResult<std::chrono::microseconds>(long long rep) const
{
    using namespace std::chrono;
    return waitForSerializedResult( \
                duration_cast<microseconds>(microseconds(rep)) );
}

template<>
str_t Job::waitForSerializedResult<std::chrono::milliseconds>(long long rep) const
{
    using namespace std::chrono;
    return waitForSerializedResult( \
                duration_cast<microseconds>(milliseconds(rep)) );
}

str_t Job::waitForSerializedResult() const
{
    return waitForSerializedResult<std::chrono::microseconds>(1);
}

}// namespace XBusLite

#ifdef XBUS_SOURCE_FOR_CLIENT_HOST

int main(int argc, char const *argv[])
{
    return XBusLite::ClientHostMain(argc, argv);
}

#endif // XBUS_SOURCE_FOR_CLIENT_HOST
