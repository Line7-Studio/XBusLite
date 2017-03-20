
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

    target_include_directories(
        ${client_name}_client PRIVATE ${xbus_lite_dir}/src)
    target_compile_definitions(
            ${client_name}_client PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)

    # TODO: add target check here??
    target_sources(${client_name} PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${client_name} PRIVATE ${xbus_lite_dir}/src/XBus.hxx)
    target_sources(${client_name} PRIVATE ${xbus_client_host_source_files})
    target_include_directories(${client_name} PRIVATE ${xbus_lite_dir}/src)

endfunction()


# export function for public use
function(xbus_lite_set_client client_name ARG_TYPE)
    if(${ARG_TYPE} STREQUAL "HOST_DIR")
        set_target_properties(${client_name}_client PROPERTIES
                                    RUNTIME_OUTPUT_DIRECTORY ${ARGN})
    elseif(${ARG_TYPE} STREQUAL "CLIENT_SRC_DIR")
        get_target_property(CLIENT_HOST_DIR ${client_name}_client
                                                RUNTIME_OUTPUT_DIRECTORY)

        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CLIENT_HOST_DIR} CLIENT_HOST_DIR)
        file(TO_NATIVE_PATH ${ARGN} CLIENT_SRC_DIR)

        if(WIN32)
            execute_process(COMMAND mkdir ${CLIENT_HOST_DIR})
            execute_process(COMMAND xcopy ${CLIENT_SRC_DIR} ${CLIENT_HOST_DIR} /E /Y)
        else()

        endif()

    elseif(${ARG_TYPE} STREQUAL "EXECUABLE_NAME")
        set_target_properties(${client_name}_client PROPERTIES
                                            RUNTIME_OUTPUT_NAME ${ARGN})
    endif()
endfunction()
