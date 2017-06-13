cmake_minimum_required (VERSION 3.4)

# define the project
project(XBusLite)

enable_testing()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(3rd/googletest)

################################################################################
include(XBusLite.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(xbus_python_executable E:/Python/3.6/x64/python)
else()
    set(xbus_python_executable python3)
endif()

# all unit test
list(APPEND xbus_unit_test "knock_knock")
list(APPEND xbus_unit_test "embed_module")
list(APPEND xbus_unit_test "create_client")
list(APPEND xbus_unit_test "task_execution")

foreach(this ${xbus_unit_test})

    add_executable(${this} test/${this}.cxx)

    xbus_add_client(${this} SOURCE test/${this}/${this}.cxx)

    xbus_set_client(${this} EXECUTABLE ${this})
    xbus_set_client(${this} EXECUTABLE_DIR xbus_client_host)

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/${this}/${this}.py)
        xbus_set_client(${this} EMBED_PYTHON_SOURCE
                        FILE ${CMAKE_CURRENT_SOURCE_DIR}/test/${this}/${this}.py
                        URL ":EmbededSource")
    endif()

    # for gtest
    target_link_libraries(${this} gtest)
    target_include_directories(${this} PRIVATE ${gtest_SOURCE_DIR}/include)

    # enable gtest color print
    add_test(${this} ${this} --gtest_color=yes)

endforeach()



################################################################################
# set our c++ compiler

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)

    set(C_FLAGS_COMM "/nologo /Zc:wchar_t- /Zc:strictStrings /MP /W3")
    set(CXX_FLAGS_COMM "/nologo /Zc:wchar_t- /Zc:strictStrings /MP /W3 /GR /EHsc")

    # enable c++14
    set(CXX_FLAGS_COMM "${CXX_FLAGS_COMM} /std:c++14")

    set(C_FLAGS_DEBUG "/DDEBUG /MDd")
    set(CXX_FLAGS_DEBUG "/DDEBUG /MDd")

    set(C_FLAGS_RELEASE "/DNDEBUG /MD")
    set(CXX_FLAGS_RELEASE "/DNDEBUG /MD")

    set(LINKER_FLAGS_COMM "/NOLOGO /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO /DEBUG")

    set(LINKER_FLAGS_DEBUG "")
    set(LINKER_FLAGS_RELEASE "")

else() # we thing its gcc or clang

    set(C_FLAGS_COMM "-Wall")
    set(CXX_FLAGS_COMM "-Wall")

    # enable c++14
    set(CXX_FLAGS_COMM "${CXX_FLAGS_COMM} -std=c++14")

    set(C_FLAGS_DEBUG "-g")
    set(CXX_FLAGS_DEBUG "-g")

    set(C_FLAGS_RELEASE "-g -O2")
    set(CXX_FLAGS_RELEASE "-g -O2")

    set(LINKER_FLAGS_COMM "")

    set(LINKER_FLAGS_DEBUG "")
    set(LINKER_FLAGS_RELEASE "")
endif()


set(C_FLAGS_DEBUG "${C_FLAGS_DEBUG} ${C_FLAGS_COMM}")
set(CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG} ${CXX_FLAGS_COMM}")

set(C_FLAGS_RELEASE "${C_FLAGS_RELEASE} ${CXX_FLAGS_COMM}")
set(CXX_FLAGS_RELEASE "${CXX_FLAGS_RELEASE} ${CXX_FLAGS_COMM}")

set(EXE_LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG} ${LINKER_FLAGS_COMM}")
set(EXE_LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} ${LINKER_FLAGS_COMM}")

# clean predefined
set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")

function(formal_and_set_flags flag_name)

    set(${flag_name} "${CMAKE_${flag_name}} ${${flag_name}}")
    string(STRIP ${${flag_name}} ${flag_name})

    if("${${flag_name}}" STREQUAL "")
        return()
    endif()

    string(REPLACE " " ";" ${flag_name} ${${flag_name}})
    list(SORT ${flag_name})
    list(REMOVE_DUPLICATES ${flag_name})
    string(REPLACE ";" " " ${flag_name} "${${flag_name}}")
    string(STRIP ${${flag_name}} ${flag_name})

    # set the CMAKE_xxx counterpart
    set(CMAKE_${flag_name} ${${flag_name}} PARENT_SCOPE)

endfunction()

formal_and_set_flags(C_FLAGS_DEBUG)
formal_and_set_flags(CXX_FLAGS_DEBUG)

formal_and_set_flags(C_FLAGS_RELEASE)
formal_and_set_flags(CXX_FLAGS_RELEASE)

formal_and_set_flags(EXE_LINKER_FLAGS_DEBUG)
formal_and_set_flags(EXE_LINKER_FLAGS_RELEASE)
