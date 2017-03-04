cmake_minimum_required (VERSION 3.4)

# define the project
project(XBusLite)

enable_testing()

# set compiler and link

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_definitions(-DXBUS_LITE_PLATFORM_DARWIN=1)
endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)

if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    add_definitions(-DXBUS_LITE_PLATFORM_FREEBSD=1)
endif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_definitions(-DXBUS_LITE_PLATFORM_WINDOWS=1)
endif(CMAKE_SYSTEM_NAME STREQUAL Windows)

set(CMAKE_CXX_STANDARD 14) # enable c++14 by default

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

    add_definitions(-DWIN32)
    add_definitions(-DUNICODE)

    set(WARNING_FLAGS "/W3")

    set(CFLAGS_COMM "/nologo /Zc:wchar_t- /Zc:strictStrings /MP ")
    set(CXXFLAGS_COMM "/nologo /Zc:wchar_t- /Zc:strictStrings /MP /GR /EHsc /std:c++14 ")

    set(CFLAGS_DEBUG "${CFLAGS_COMM} /MDd ")
    set(CXXFLAGS_DEBUG "${CXXFLAGS_COMM} /MDd ")

    set(CFLAGS_RELEASE "${CFLAGS_COMM} /MD ")
    set(CXXFLAGS_RELEAE "${CXXFLAGS_COMM} /MD ")

    set(LINKFLAGS_COMM "/NOLOGO /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO /DEBUG ")

    set(LINKFLAGS_DEBUG "${LINKFLAGS_COMM}")
    set(LINKFLAGS_RELEASE "${LINKFLAGS_COMM}")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CFLAGS_DEBUG} ${WARNING_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXXFLAGS_DEBUG} ${WARNING_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${LINKFLAGS_DEBUG}")

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CFLAGS_RELEASE} ${WARNING_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXXFLAGS_RELEAE} ${WARNING_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${LINKFLAGS_RELEASE}")

else() # we thing its gcc or clang

    add_definitions(-DNDEBUG)

    set(WARNING_FLAGS "-Wall")

    set(CFLAGS "-O3 -g")
    set(CXXFLAGS "-std=c++14 -g")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS} ${WARNING_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXXFLAGS} ${WARNING_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKFLAGS}")

endif()

# add googletest
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(3rd/googletest)

# set xbus all files
set(xbus_src_files src/XBus.hxx src/XBus.cxx)

# all unit test
file(GLOB files LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cxx)
foreach(this_test_src_file ${files})
    get_filename_component(file_name ${this_test_src_file} NAME)
    if(NOT ${file_name} MATCHES "^([a-zA-Z_]+).cxx$")
        continue()
    endif()

    message(">>>> Test: ${this_test_src_file}")

    get_filename_component(file_dir ${this_test_src_file} DIRECTORY)
    get_filename_component(file_name_we ${this_test_src_file} NAME_WE)

    set(this ${file_name_we})

    # for the client host, for other project we just need change this
    set(client_host_src_file ${file_dir}/${file_name_we}.xbus.cxx)
    # this is where the host will keep in
    set(client_host_dir xbus_client_host)
    # this is which the host will be named
    set(client_host_name ${this})
    # for the server itself, for other project we just need change this
    set(server_src_files ${this_test_src_file})

    add_executable(${this}_client ${xbus_src_files} ${client_host_src_file})
    target_include_directories(${this}_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(${this}_client PRIVATE XBUS_SOURCE_FOR_CLIENT_HOST)
    if(client_host_dir)
        set_target_properties(${this}_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${client_host_dir})
    endif()
    set_target_properties(${this}_client PROPERTIES RUNTIME_OUTPUT_NAME ${client_host_name})

    add_executable(${this} ${xbus_src_files} ${client_host_src_file} ${server_src_files})
    target_include_directories(${this} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    # if not gtest need, comment next two lines
    target_link_libraries(${this} gtest)
    target_include_directories(${this} PRIVATE ${gtest_SOURCE_DIR}/include)

    add_test(${this} ${this} --gtest_color=yes)
endforeach()
