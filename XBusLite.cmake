
# store this dir for futher use
set(xbus_lite_dir ${CMAKE_CURRENT_LIST_DIR})

# set compiler and link
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_definitions(-DXBUS_LITE_PLATFORM_LINUX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_definitions(-DXBUS_LITE_PLATFORM_DARWIN=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    add_definitions(-DXBUS_LITE_PLATFORM_FREEBSD=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_definitions(-DXBUS_LITE_PLATFORM_WINDOWS=1)
endif()


# export function for public use
function(xbus_lite_add_client client_name)
    message(STATUS "XBusLite Add Client: ${client_name}")

    # TODO: add file check here
    set(xbus_client_host_source_files "${ARGN}")

    add_executable(${client_name}_client ${xbus_client_host_source_files})
    target_sources(${client_name}_client PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${client_name}_client PRIVATE ${xbus_lite_dir}/src/XBus.hxx)

    target_include_directories(${client_name}_client PRIVATE ${xbus_lite_dir}/src)
    target_compile_definitions(${client_name}_client PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)

    # TODO: add target check here??
    target_sources(${client_name} PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${client_name} PRIVATE ${xbus_lite_dir}/src/XBus.hxx)
    target_sources(${client_name} PRIVATE ${xbus_client_host_source_files})
    target_include_directories(${client_name} PRIVATE ${xbus_lite_dir}/src)

    get_target_property(SERVER_HOST_NAME ${client_name} RUNTIME_OUTPUT_NAME)

endfunction()


# export function for public use
function(xbus_lite_set_client client_name ARG_TYPE)

    if(${ARG_TYPE} STREQUAL "EXECUTABLE_DIR")
        set_target_properties(${client_name}_client
                                PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARGN})
        set(${client_name}_client_EXECUTABLE_DIR ${ARGN} PARENT_SCOPE)
    endif()

    if(${ARG_TYPE} STREQUAL "EXECUTABLE_NAME")
        set_target_properties(${client_name}_client
                                PROPERTIES RUNTIME_OUTPUT_NAME ${ARGN})
        set(${client_name}_client_EXECUTABLE_NAME ${ARGN} PARENT_SCOPE)
    endif()

    if(${ARG_TYPE} STREQUAL "SERVER_CONFIG_FILE")
        file(WRITE ${ARGN} "[XBUS]\n")

        set(host_file "HOST_FILE=")
        if(${client_name}_client_EXECUTABLE_DIR})
            set(client_exe_dir ${${client_name}_client_EXECUTABLE_DIR})
            set(host_file  "HOST_FILE=${client_exe_dir}/")
        endif()

        set(client_exe_name ${${client_name}_client_EXECUTABLE_NAME})
        set(host_file ${host_file}${client_exe_name})

        file(TO_NATIVE_PATH ${host_file} host_file)

        if(WIN32)
            file(APPEND ${ARGN} "${host_file}.exe\n")
        else()
            file(APPEND ${ARGN} "${host_file}\n")
        endif()

        file(APPEND ${ARGN} "PYTHON_RUNTIME=${XBUS_PYTHON_RUNTIME}\n")
    endif()

    if(${ARG_TYPE} STREQUAL "SOURCE_COPY_TO")
        file(MAKE_DIRECTORY ${ARGN})
        set(${client_name}_client_SOURCE_COPY_TO ${ARGN} PARENT_SCOPE)
    endif()


    if(${ARG_TYPE} STREQUAL "SOURCE_COPY_FROM")

        file(TO_NATIVE_PATH ${${client_name}_client_SOURCE_COPY_TO} COPY_TO_DIR)
        file(TO_NATIVE_PATH ${ARGN} COPY_FROM_DIR)

        if(WIN32)
            execute_process(COMMAND xcopy ${COPY_FROM_DIR} ${COPY_TO_DIR} /E /Y)
        else()
            execute_process(COMMAND cp -R ${COPY_FROM_DIR}/ ${COPY_TO_DIR}/)
        endif()

    endif()


endfunction()
