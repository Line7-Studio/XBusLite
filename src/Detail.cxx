#include "Detail.hxx"

namespace XBusLite
{


std::map<str_t, process_handle_t>& ClientNameToChildProcessHandle()
{
    static std::map<str_t, process_handle_t> kv;
    return kv;
}

std::map<str_t, client_init_function_t>& ClientNameToInitFunction()
{
    static std::map<str_t, client_init_function_t> kv;
    return kv;
}

std::map<str_t, embedded_c_function_t>& EmbededFunctionNameToFunction()
{
    static std::map<str_t, embedded_c_function_t> kv;
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

file_path_t check_and_formal_file_path(const file_path_t& file_path)
{
#ifdef XBUS_LITE_PLATFORM_WINDOWS
    const auto attr = ::GetFileAttributes(file_path.c_str());
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
        printf("%s ::realpath error: %s\n", __FUNCTION__, ::strerror(errno));
        throw std::runtime_error(str_t(__FUNCTION__)+" ::realpath failed");
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

#ifdef XBUS_LITE_PLATFORM_LINUX
    char buffer[PATH_MAX+1];
    if( ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1) == -1 ) {
      throw std::string("::readlink(/proc/self/exe) failed");
    }
    result = file_path_t(buffer);
#endif //XBUS_LITE_PLATFORM_LINUX

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
        printf("CreateClient Try to Create None Registered Client %s\n", client_name.c_str());
        throw Error::NoneRegisteredClient(client_name);
    }

    auto shared_mem = ClientNameToServerSideSharedMem(new_client_name);
    auto communication_tube = ClientNameToCommunicationTube(new_client_name);

    // then create client process
#ifdef XBUS_LITE_PLATFORM_WINDOWS

    PROCESS_INFORMATION prcInfo = {};
    STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,             \
                                 (DWORD)CW_USEDEFAULT, (DWORD)CW_USEDEFAULT, \
                                 (DWORD)CW_USEDEFAULT, (DWORD)CW_USEDEFAULT, \
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                \
                               };

    DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
    creation_flags |= CREATE_BREAKAWAY_FROM_JOB;
    // creation_flags |= DETACHED_PROCESS;
    creation_flags |= CREATE_NEW_CONSOLE;

    auto client_host_executable = get_this_executable_located_dir() \
                                        + L"\\"+ ClientHostFilePath();

    // 1. do not forget the space between arguments
    // 2. need qute for white space chars
    std::wstringstream args;
    args << L"\"" << client_host_executable << L"\" ";
    args << L"--shared_mem="<< shared_mem->name() << L" ";

    // According MSDN, CreateProcess unicode version
    // here we maybe have a bug with const_cast
    auto success = ::CreateProcess( client_host_executable.c_str(), \
                                    const_cast<wchar_t*>(args.str().c_str()), \
                                    NULL, NULL, TRUE, creation_flags, 0, NULL, \
                                    &startupInfo, &prcInfo );

    if( success == 0 ){
        printf("::CreateProcess Failed %lu\n", ::GetLastError());
        throw std::runtime_error(__FUNCTION__" ::CreateProcess Failed");
    }

    auto process_handle = prcInfo.hProcess;

    if( 0 == ::AssignProcessToJobObject(JOB_SESSION_OBJECT, process_handle) )
    {
        printf("::AssignProcessToJobObject Failed %lu\n", ::GetLastError());
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

    #ifdef XBUS_LITE_PLATFORM_DARWIN
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->request.for_read);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->request.for_write);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->response.for_read);
    posix_spawn_file_actions_addinherit_np(&file_actions, \
                                    communication_tube->response.for_write);
    #else // Not on macOS

    #endif // XBUS_LITE_PLATFORM_DARWIN

    /*
    posix_spawn_file_actions_addopen(&file_actions, \
                            STDOUT_FILENO, "/dev/null", O_WRONLY, 0);

    posix_spawn_file_actions_addopen(&file_actions, \
                            STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    */

    auto success = ::posix_spawn(&process_handle, \
                        client_host_executable.c_str(), \
                        &file_actions, nullptr, spawned_args, nullptr);
                        // &file_actions, nullptr, spawned_args, environ);

    posix_spawn_file_actions_destroy(&file_actions);

    if( success != 0 ){
        printf("%s ::posix_spawn failed\n", __FUNCTION__);
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
    auto const shared_mem = ClientNameToServerSideSharedMem(client_name);

    auto const state = reinterpret_cast<ClientInitState*>(shared_mem->data());

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
            // TODO: should NOT be std::runtime_error
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

std::pair<Job::State::Enum, std::vector<char>>
    ExecuteJob(const std::vector<char>& fun_name);

std::pair<Job::State::Enum, std::vector<char>>
    ExecuteJob(const std::vector<char>& fun_name, const std::vector<char>& fun_args);

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
                // TODO: should NOT be std::runtime_error
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

#ifdef XBUS_LITE_PLATFORM_LINUX
// kill the client when server process terminate
void* child_thread_handle_server_termination(void* /*arguments*/)
{
    // int kq = kqueue();
    // struct kevent kev;
    // EV_SET(&kev, getppid(), EVFILT_PROC, EV_ADD|EV_ENABLE, NOTE_EXIT, 0, NULL);

    // (void) kevent(kq, &kev, 1, &kev, 1, NULL);
    // exit(EXIT_SUCCESS);

    return nullptr;
}
#endif // XBUS_LITE_PLATFORM_LINUX

bool InitPythonRuntime(int argc, char* argv[]);

int ClientHostMain(int argc, char* argv[])
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

    const auto state = tube->response.read<uint32_t>();

    return Job::State::Enum(state);
}

str_t Job::fetchSerializedResult() const
{
    auto tube = ClientNameToCommunicationTube(client_name());

    tube->request.write<uint32_t>(RequestType::FETCH_JOB_RESULT);
    tube->request.write<uint32_t>(id());

    const auto result = tube->response.read<str_t>();

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

#ifndef XBUS_SOURCE_FOR_CLIENT_HOST_NO_MAIN

int main(int argc, char* argv[])
{
    return XBusLite::ClientHostMain(argc, argv);
}

#endif // XBUS_SOURCE_FOR_CLIENT_HOST_NO_MAIN

#endif // XBUS_SOURCE_FOR_CLIENT_HOST