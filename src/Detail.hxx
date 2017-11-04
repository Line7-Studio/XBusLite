#pragma once

#include <XBus.hxx>

#include <cstddef> // std::ptrdiff_t
#include <cstring> // std::strlen

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

#include <fstream>

#ifdef XBUS_LITE_PLATFORM_WINDOWS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif //XBUS_LITE_PLATFORM_WINDOWS

#if defined(XBUS_LITE_PLATFORM_LINUX) \
 || defined(XBUS_LITE_PLATFORM_DARWIN) \
 || defined(XBUS_LITE_PLATFORM_FREEBSD)
    #include <dlfcn.h> // dynamic load functions for python
    #include <spawn.h> // for posix spawn functions
    #include <unistd.h> // for read & write functions
    #include <fcntl.h>  // for O_* constants
    #include <sys/stat.h> // for S_* constants
    #include <sys/mman.h> // for shm_XXX functions
#endif

#ifdef XBUS_LITE_PLATFORM_LINUX  // shm_XXX functions
    #include <linux/limits.h> // for PATH_MAX
#endif //XBUS_LITE_PLATFORM_LINUX

#ifdef XBUS_LITE_PLATFORM_DARWIN
    #include <sys/syslimits.h> // for PATH_MAX
    #include <sys/types.h> // for kqueue
    #include <sys/event.h> // for kqueue
    #include <sys/time.h>  // for kqueue
    #include <mach-o/dyld.h> // for module path
#endif //XBUS_LITE_PLATFORM_DARWIN

#ifdef XBUS_LITE_PLATFORM_FREEBSD  // shm_XXX functions

#endif //XBUS_LITE_PLATFORM_FREEBSD

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
public:
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
};

class ServerSideSharedMem: public IFixedSizeSharedMem
{
public:
    ServerSideSharedMem(size_t size)
    {
        // using random object name to avoid confilicte
        // Mac OS only support max length 31
        m_name = generate_a_random_file_name(16);
    #ifdef XBUS_LITE_PLATFORM_WINDOWS
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
    #else // Not On Windows
        auto required_size = sizeof(size_t) + size;

        m_handle = ::shm_open(m_name.data(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if( m_handle == -1 ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::shm_open failed");
        }

        if( ::ftruncate(m_handle, required_size) != 0 ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::ftruncate failed");
        }

        m_buffer = (char*)::mmap(0, required_size, PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        if( m_buffer == MAP_FAILED ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::mmap failed");
        }

        // store the size info
        *((size_t*)m_buffer) = required_size;
    #endif // XBUS_LITE_PLATFORM_WINDOWS
    }

public:
    ~ServerSideSharedMem()
    {
    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        if( 0 == ::UnmapViewOfFile(m_buffer) ){
            perror(__FUNCTION__" ::UnmapViewOfFile Failed");
        }
        if( 0 == ::CloseHandle(m_handle) ){
            perror(__FUNCTION__" ::CloseHandle Failed");
        }
    #else // Not On Windows
        auto required_size = *((size_t*)m_buffer);

        if( ::munmap(m_buffer, required_size) ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::munmap failed");
        }
        if( ::shm_unlink(m_name.c_str()) != 0 ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::shm_unlink failed");
        }
    #endif // XBUS_LITE_PLATFORM_WINDOWS
    }
};

class ClientSideSharedMem: public IFixedSizeSharedMem
{
public:
    ClientSideSharedMem(const file_path_t& name)
    {
        m_name = name;
    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        m_handle = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name.data());

        if( m_handle == NULL ){
            throw std::runtime_error(__FUNCTION__" ::OpenFileMapping Failed");
        }

        m_buffer = (char*)::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if( m_buffer == NULL ){
            throw std::runtime_error(__FUNCTION__" ::MapViewOfFile Failed");
        }
    #else // Not On Windows
        m_handle = ::shm_open(name.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);

        if( m_handle == -1 ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::shm_open failed");
        }

        auto buffer = (char*)::mmap(NULL, sizeof(size_t), PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        auto mem_size = *((size_t*)buffer);

        m_buffer = (char*)::mmap(NULL, mem_size, PROT_READ|PROT_WRITE, \
                                                 MAP_SHARED, m_handle, 0);

        if( m_buffer == MAP_FAILED ){
            throw std::runtime_error(str_t(__FUNCTION__)+" ::mmap failed");
       }
    #endif // XBUS_LITE_PLATFORM_WINDOWS
    }

public:
    ~ClientSideSharedMem()
    {
    #ifdef XBUS_LITE_PLATFORM_WINDOWS
        // TODO:
    #else // Not On Windows
        if( 0 != ::munmap(m_buffer, *((size_t*)m_buffer)) ) {
            printf("%s ::munmap failed\n", __FUNCTION__);
        }
        if( 0 != ::close(m_handle) ){
            printf("%s ::close failed\n", __FUNCTION__);
        }
    #endif // XBUS_LITE_PLATFORM_WINDOWS
    }
};

template<typename T>
T read_pipe_for_value(pipe_handle_t pipe_handle)
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
void write_pipe_for_value(pipe_handle_t pipe_handle, const T value)
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
T read_pipe_for_buffer(pipe_handle_t pipe_handle)
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

    if( read_pipe(data_buffer, buffer_size) != ssize_t(buffer_size) )
    {
        printf("%s ::read failed 2\n", __FUNCTION__);
        throw std::runtime_error(str_t(__FUNCTION__)+" ::read failed 2");
    }
#endif // XBUS_LITE_PLATFORM_WINDOWS

    return buffer_data;
}

template<typename T>
void write_pipe_for_buffer(pipe_handle_t pipe_handle, const T& buffer)
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

    if( write_pipe(data_buffer, buffer_size) != ssize_t(buffer_size) )
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

}// namespace XBusLite