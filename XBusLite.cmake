

# store this dir for futher use
set(xbus_lite_dir ${CMAKE_CURRENT_LIST_DIR})


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
function(xbus_add_client xbus_server_name KEYWORD_SRC)
    # check the host is defined before add client
    if(NOT TARGET ${xbus_server_name})
        message(FATAL_ERROR "must call xbus_set_client_host after a existed target")
    endif()

    if(NOT ${KEYWORD_SRC} STREQUAL "SOURCE")
        message(FATAL_ERROR "must call xbus_set_client_host with keyword `SOURCE`")
    endif()

    message(STATUS "XBusLite Add Client: ${xbus_server_name}")

    # check file exist here
    set(xbus_client_host_source_files ${ARGN})
    foreach(var ${xbus_client_host_source_files})
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${var})
            message(FATAL_ERROR "xbus_set_client_host add none exist file: ${var}")
        endif()
    endforeach()

    add_executable(${xbus_server_name}_xbus_client_host ${xbus_client_host_source_files})

    # basic settings for the host
    target_sources(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src/XBus.hxx)
    target_sources(${xbus_server_name} PRIVATE ${xbus_client_host_source_files})
    target_include_directories(${xbus_server_name} PRIVATE ${xbus_lite_dir}/src)

    # basic settings for the client
    target_sources(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src/XBus.cxx)
    target_sources(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src/XBus.hxx)
    target_include_directories(${xbus_server_name}_xbus_client_host PRIVATE ${xbus_lite_dir}/src)
    target_compile_definitions(${xbus_server_name}_xbus_client_host PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)

    # make sure the client executeable file exist relative to the host
    get_target_property(server_host_name ${xbus_server_name} OUTPUT_NAME)
    get_target_property(server_host_dir ${xbus_server_name} RUNTIME_OUTPUT_DIRECTORY)

    # by default cmake will not set this property,
    # but the default value is the same as ${xbus_server_name}
    if(NOT server_host_name)
        set(server_host_name ${xbus_server_name})
    endif()

    # by default, we make the client's file name like ${host-name}-xbus-client
    set_target_properties(${xbus_server_name}_xbus_client_host PROPERTIES
                                RUNTIME_OUTPUT_NAME ${server_host_name}-xbus-client)

    # this is the default dir, eg. same as the server
    if(server_host_dir) # well, some one set the host output dir already
        # by default, we make the client exist with the host directory
        set_target_properties(${xbus_server_name}_xbus_client_host PROPERTIES
                                    RUNTIME_OUTPUT_DIRECTORY ${server_host_dir})
    else()
        # ??
    endif()


    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        # Build an executable with a WinMain entry point on windows.
        get_target_property(server_is_gui_build ${xbus_server_name} WIN32_EXECUTABLE)
        if(server_is_gui_build)
            set_target_properties(${xbus_server_name}_xbus_client_host PROPERTIES
                                    WIN32_EXECUTABLE ${server_is_gui_build})
        endif()
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(${xbus_server_name} ${librt} ${libdl} ${libpthread})
        target_link_libraries(${xbus_server_name}_xbus_client_host ${librt} ${libdl} ${libpthread})
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
         # TODO: ???
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        get_target_property(is_macos_bundle ${xbus_server_name} MACOSX_BUNDLE)
        if(NOT server_host_dir)
            message(FATAL_ERROR "Fucked by CMake?? need find server_host_dir first!")
        endif()

        if(${is_macos_bundle})
            set_target_properties(${xbus_server_name}_xbus_client_host PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${server_host_dir}/${xbus_server_name}.app/Contents/MacOS)
        endif()
    endif()

endfunction()


# export function for public use
function(xbus_set_client xbus_server_name ARG_TYPE)

    if(${ARG_TYPE} STREQUAL "EXECUTABLE")
        get_target_property(var ${xbus_server_name} OUTPUT_NAME)

        if(NOT var)
            set(the_xbus_server_executable_name ${xbus_server_name})
        else()
            set(the_xbus_server_executable_name ${var})
        endif()

        get_target_property(var ${xbus_server_name} RUNTIME_OUTPUT_DIRECTORY)

        if(NOT var)
            set(the_xbus_server_executable_dir_is_default TRUE)
        else()
            set(the_xbus_server_executable_dir_is_default FALSE)
        endif()

        if(DEFINED ${xbus_server_name}_client_EXECUTABLE_DIR)
            # we call xbus_set_client(EXECUTABLE_DIR) already
            # message("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
        else()
            # message("NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN")
            if(${the_xbus_server_executable_dir_is_default} STREQUAL TRUE)
                if(${ARGN} STREQUAL ${the_xbus_server_executable_name})
                    message(WARNING
                    "The client is set the same name as the host `${xbus_server_name}`!! \
                    This will lead the client's executable file overwrite the host file!! \
                    You should set the client with an other name, \
                    or call `xbus_set_client` EXECUTABLE_DIR to chanage the client's output dir`"
                    )
                endif()
            else()
                # if not the default dir, we don't give it a fuck
            endif()
        endif()

        set_target_properties(${xbus_server_name}_xbus_client_host
                                PROPERTIES RUNTIME_OUTPUT_NAME ${ARGN})
        set(${xbus_server_name}_client_EXECUTABLE_NAME ${ARGN} PARENT_SCOPE)
    endif()


    if(${ARG_TYPE} STREQUAL "EXECUTABLE_DIR")
        get_target_property(var ${xbus_server_name} RUNTIME_OUTPUT_DIRECTORY)

        if(NOT var)
            set(the_xbus_server_executable_dir_is_default TRUE)
        else()
            set(the_xbus_server_executable_dir_is_default FALSE)
            set(server_host_dir ${var})
        endif()

        if(${the_xbus_server_executable_dir_is_default} STREQUAL TRUE)
            set_target_properties(${xbus_server_name}_xbus_client_host
                    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARGN})
            set(${xbus_server_name}_client_EXECUTABLE_DIR ${ARGN} PARENT_SCOPE)
        else()
            set_target_properties(${xbus_server_name}_xbus_client_host
                    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${server_host_dir}/${ARGN})
            set(${xbus_server_name}_client_EXECUTABLE_DIR ${server_host_dir}/${ARGN} PARENT_SCOPE)
        endif()

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


    if(${ARG_TYPE} STREQUAL "EMBED_PYTHON_SOURCE")
        xbus_embed_source_code(${xbus_server_name}_xbus_client_host
            ${ARGN}
            )
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
