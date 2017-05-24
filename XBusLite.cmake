

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
function(xbus_add_client_host xbus_server_name keyword_src)
    if(NOT TARGET ${xbus_server_name})
        message(FATAL_ERROR "must call xbus_lite_add_host after a existed target")
    endif()

    if(NOT ${keyword_src} STREQUAL "SOURCE")
        message(FATAL_ERROR "must call xbus_lite_add_host with keyword `SOURCE`")
    endif()

    message(STATUS "XBusLite Add Client: ${xbus_server_name}")

    get_target_property(server_is_win32 ${xbus_server_name} WIN32_EXECUTABLE)

    # check file exist here
    set(xbus_client_host_source_files ${ARGN})
    foreach(var ${xbus_client_host_source_files})
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${var})
            message(FATAL_ERROR "xbus_lite_add_host add none exist file: ${var}")
        endif()
    endforeach()

    if(server_is_win32)
        add_executable(${xbus_server_name}_xbus_client_host WIN32 ${xbus_client_host_source_files})
    else()
        add_executable(${xbus_server_name}_xbus_client_host ${xbus_client_host_source_files})
    endif()


    target_sources(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src/XBus.hxx)

    target_include_directories(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src)
    target_compile_definitions(${xbus_server_name}_xbus_client_host PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)

    target_sources(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src/XBus.hxx)
    target_sources(${xbus_server_name} PRIVATE ${xbus_client_host_source_files})
    target_include_directories(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src)

    get_target_property(SERVER_HOST_NAME ${xbus_server_name} RUNTIME_OUTPUT_NAME)

    get_property(is_macos_bundle TARGET ${xbus_server_name} PROPERTY MACOSX_BUNDLE)
    if(${is_macos_bundle})
        set_target_properties(${xbus_server_name}_xbus_client_host
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${xbus_server_name}.app/Contents/MacOS)
    endif()

endfunction()


# export function for public use
function(xbus_set_client_host xbus_server_name ARG_TYPE)

    if(${ARG_TYPE} STREQUAL "EXECUTABLE_DIR")
        set_target_properties(${xbus_server_name}_xbus_client_host
                                PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARGN})
        set(${xbus_server_name}_client_EXECUTABLE_DIR ${ARGN} PARENT_SCOPE)
    endif()

    if(${ARG_TYPE} STREQUAL "EXECUTABLE_NAME")
        set_target_properties(${xbus_server_name}_xbus_client_host
                                PROPERTIES RUNTIME_OUTPUT_NAME ${ARGN})
        set(${xbus_server_name}_client_EXECUTABLE_NAME ${ARGN} PARENT_SCOPE)
    endif()

    if(${ARG_TYPE} STREQUAL "SERVER_CONFIG_FILE")
        file(WRITE ${ARGN} "[XBUS]\n")

        set(host_file "HOST_FILE=")
        if(${xbus_server_name}_client_EXECUTABLE_DIR})
            set(client_exe_dir ${${xbus_server_name}_client_EXECUTABLE_DIR})
            set(host_file  "HOST_FILE=${client_exe_dir}/")
        endif()

        set(client_exe_name ${${xbus_server_name}_client_EXECUTABLE_NAME})
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
        set(${xbus_server_name}_client_SOURCE_COPY_TO ${ARGN} PARENT_SCOPE)
    endif()


    if(${ARG_TYPE} STREQUAL "SOURCE_COPY_FROM")

        file(TO_NATIVE_PATH ${${xbus_server_name}_client_SOURCE_COPY_TO} COPY_TO_DIR)
        file(TO_NATIVE_PATH ${ARGN} COPY_FROM_DIR)

        if(WIN32)
            execute_process(COMMAND xcopy ${COPY_FROM_DIR} ${COPY_TO_DIR} /E /Y)
        else()
            execute_process(COMMAND cp -R ${COPY_FROM_DIR}/ ${COPY_TO_DIR}/)
        endif()

    endif()

endfunction()
