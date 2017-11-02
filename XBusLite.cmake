

# store this variable for futher use
set(xbus_lite_dir ${CMAKE_CURRENT_LIST_DIR})

execute_process(COMMAND ${xbus_python_executable} --version OUTPUT_VARIABLE xbus_python_version)
string(STRIP ${xbus_python_version} xbus_python_version)
string(REPLACE " " "." xbus_python_version ${xbus_python_version})
string(REPLACE "." ";" xbus_python_version ${xbus_python_version})

list(GET xbus_python_version 1 xbus_python_version_major)
list(GET xbus_python_version 2 xbus_python_version_minor)
list(GET xbus_python_version 3 xbus_python_version_maintenance)
add_definitions(-DXBUS_PYTHON_VERSION_MAJOR=${xbus_python_version_major})
add_definitions(-DXBUS_PYTHON_VERSION_MINOR=${xbus_python_version_minor})
add_definitions(-DXBUS_PYTHON_VERSION_MAINTENANCE=${xbus_python_version_maintenance})

add_definitions(-DXBUS_PYTHON_ZIP_PACKAGE_NAME="python${xbus_python_version_major}${xbus_python_version_minor}.zip")
add_definitions(-DXBUS_PYTHON_LIB_VERSION_PREFIX="python${xbus_python_version_major}.${xbus_python_version_minor}")

# set compiler and link
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_definitions(-DXBUS_LITE_PLATFORM_LINUX=1)
    find_library(libdl dl)
    find_library(librt rt)
    find_library(libpthread pthread)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_definitions(-DXBUS_LITE_PLATFORM_DARWIN=1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    add_definitions(-DXBUS_LITE_PLATFORM_FREEBSD=1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_definitions(-DXBUS_LITE_PLATFORM_WINDOWS=1)
endif()

# export function for public use
function(xbus_embed_source_code target_name)

    list(LENGTH ARGN argn_lenght)

    set(error_head "xbus_embed_source_code EMBED_PYTHON_SOURCE")

    if(NOT ${argn_lenght} EQUAL 4)
        message(FATAL_ERROR "${error_head} wrong arguments length")
    endif()

    list(GET ARGN 0 ARGN_0)
    list(GET ARGN 1 embed_python_source_file)
    list(GET ARGN 2 ARGN_2)
    list(GET ARGN 3 embed_python_source_url)

    if(NOT ${ARGN_0} STREQUAL "FILE")
        message(FATAL_ERROR "${error_head} wrong arguments at index 0")
    endif()

    if(NOT ${ARGN_2} STREQUAL "URL")
        message(FATAL_ERROR "${error_head} wrong arguments at index 2")
    endif()

    get_filename_component(input_file_path ${embed_python_source_file} ABSOLUTE)

    if(NOT EXISTS ${input_file_path})
        message(FATAL_ERROR "${error_head} can not find file ${input_file_path}")
    endif()

    set(base_output_dir ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})
    set(output_dir ${base_output_dir}/xbus_lite.dir/bc2so.dir/${target_name}/)

    # create generated store file path
    file(RELATIVE_PATH relative_file_path ${CMAKE_CURRENT_SOURCE_DIR} ${input_file_path})

    # generated file full path
    get_filename_component(tmp_abs_path ${output_dir}/${relative_file_path} ABSOLUTE)

    get_filename_component(generated_file_dirctory ${tmp_abs_path} DIRECTORY)
    get_filename_component(generated_file_name ${tmp_abs_path} NAME_WE)

    set(generated_file_full_path "${generated_file_dirctory}/${generated_file_name}.cxx")

    string(REPLACE "." "_" generated_depend_name ${generated_file_full_path})
    string(REPLACE "/" "_" generated_depend_name ${generated_depend_name})
    string(REPLACE ":" "_" generated_depend_name ${generated_depend_name})

    add_custom_command(
        OUTPUT    ${generated_file_full_path}
        COMMAND   ${xbus_python_executable}
        ARGS      ${xbus_lite_dir}/bin/bc2so.py
                  --file ${input_file_path}
                  --name ${embed_python_source_url}
                  --output ${generated_file_full_path}
        DEPENDS   ${input_file_path}
        VERBATIM
    )

    add_custom_target(${generated_depend_name}
        DEPENDS ${generated_file_full_path}
    )

    target_sources(${target_name} PRIVATE ${generated_file_full_path})

endfunction()


# export function for public use
function(xbus_embed_source_code_list target_name)

    list(LENGTH ARGN argn_lenght)

    while(NOT argn_lenght EQUAL 0)
        list(GET ARGN 0 arg_0)
        list(GET ARGN 1 arg_1)
        list(GET ARGN 2 arg_2)
        list(GET ARGN 3 arg_3)
        list(REMOVE_AT ARGN 0 1 2 3)

        xbus_embed_source_code(${target_name} ${arg_0} ${arg_1} ${arg_2} ${arg_3})

        list(LENGTH ARGN argn_lenght)
    endwhile()

endfunction()

# export function for public use
function(xbus_set target_name KEYWORD_MODE)
    # check the host is defined before add client
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "must call xbus_handle_depends after a existed target")
    endif()

    # basic settings for the host
    target_sources(${target_name} PRIVATE ${xbus_lite_dir}/src/Detail.hxx)
    target_sources(${target_name} PRIVATE ${xbus_lite_dir}/src/Detail.cxx)
    target_sources(${target_name} PRIVATE ${xbus_lite_dir}/src/Python.cxx)

    target_include_directories(${target_name} PRIVATE ${xbus_lite_dir}/inc)

    if(${KEYWORD_MODE} STREQUAL "CLIENT")
        target_compile_definitions(${target_name} PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)
    elseif(${KEYWORD_MODE} STREQUAL "SERVER")
        # pass
    else()
        message(FATAL_ERROR "xbus_handle_depends KEYWORD_MODE must be CLIENT or SERVER")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        # Build an executable with a WinMain entry point on windows.
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(${target_name} ${librt} ${libdl} ${libpthread})
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
        # TODO: ???
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        # TODO: ???
    endif()

endfunction()

