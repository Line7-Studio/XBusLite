
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
    target_include_directories(${client_name} PRIVATE ${xbus_lite_dir}/src)

endfunction()


# export function for public use
function(xbus_lite_set_client_executable_dir client_name client_host_dir)
    set_target_properties(${client_name}_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${client_host_dir})
endfunction()

function(xbus_lite_set_client_executable_name client_name chient_host_name)
    set_target_properties(${client_name}_client PROPERTIES RUNTIME_OUTPUT_NAME ${client_host_name})
endfunction()
