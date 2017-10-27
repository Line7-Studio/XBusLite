#! /usr/bin/env bash

XBUS_BUILD_TYPE=release

################################################################################
os_check()
{
    case `uname` in
        Linux)
            current_date="date +%s%3N"
            os_name=Linux
            ;;
        Darwin)
            current_date="gdate +%s%3N" # use gnu date
            os_name=macOS
            ;;
        FreeBSD)
            current_date="date +%s000" # bsd date ???
            os_name=FreeBSD
            ;;
        *_NT-*)
            current_date="date +%s%3N"
            os_name=Windows
            ;;
        *)
            echo "none support os"
            exit -1
            ;;
    esac
    build_dir="tmp/build_$os_name"
}
os_check

################################################################################
function check_compiler_tools()
{
    if [ "$os_name" = "Windows" ]; then
        hash cl &> /dev/null
        if [ $? -eq 0 ] ; then
            if [[ `cl 2>&1` =~ .*x64.* ]]; then
                export for_64bit=true
            fi
            have_cxx=true
            # tell cmake select msvc toolset
            export cc=cl
            export cxx=cl
            export ld=link
        fi
    else
        hash g++ &> /dev/null
        if [ $? -eq 0 ] ; then
            have_cxx=true
        fi

        hash clang++ &> /dev/null
        if [ $? -eq 0 ] ; then
            have_cxx=true
        fi
    fi

    if [ ! $have_cxx ] ; then
        echo "c++ compiler not found in path"
        exit -1
    fi

    hash cmake &> /dev/null
    if [ $? -eq 1 ] ; then
        echo "cmake not found in path"
        exit -1
    fi

    hash ninja &> /dev/null
    if [ $? -eq 1 ] ; then
        echo "ninja-build not found in path"
        exit -1
    fi
}
check_compiler_tools

pushd `dirname $0` > /dev/null
this_script_located_dir=`pwd`

################################################################################
print_help_doc()
{
    echo " 0: init    [init xbus project need python runtime  ]"
    echo " 2: clean   [delete all generated files             ]"
    echo " 1: build   [build the application                  ]"
    echo " 3: test    [run xbus unit tests                    ]"
    echo " 4: pack    [pack xbus framework                    ]"
    echo " 5: help    [print help document                    ]"
    exit 0
}

init_xbus()
{
    ${this_script_located_dir}/bin/python_runtime_pack/main.sh build
}

build_xbus()
{
    cmake_args="-G Ninja"
    cmake_args="$cmake_args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    if [ "$XBUS_BUILD_TYPE" == "release" ]; then
        cmake_args="$cmake_args -DCMAKE_BUILD_TYPE=Release"
    else
        cmake_args="$cmake_args -DCMAKE_BUILD_TYPE=Debug"
    fi

    if [ "$os_name" = "Windows" ]; then
        if [[ $for_64bit ]]; then
            python_executable=bin/python_runtime_pack/dist/$os_name/x64/python.exe
        else
            python_executable=bin/python_runtime_pack/dist/$os_name/x32/python.exe
        fi
        cmake_args="$cmake_args -DXBUS_PYTHON_EXECUTABLE=$python_executable"
    else
        python_executable=bin/python_runtime_pack/dist/$os_name/bin/python3
        cmake_args="$cmake_args -DXBUS_PYTHON_EXECUTABLE=$python_executable"
    fi

    mkdir -p $build_dir
    pushd $build_dir >/dev/null

    echo -e 'generate cmake files'
    cmake $cmake_args ../..
    echo "ninja build"
    ninja

    popd > /dev/null
}

clean_xbus()
{
    rm -rf tmp
}

test_xbus()
{
    pushd $build_dir >/dev/null
    ctest -V
    popd > /dev/null
}

pack_xbus()
{
    # ${this_script_located_dir}/bin/python_runtime_pack/main.sh tense $2
    ${this_script_located_dir}/bin/python_runtime_pack/main.sh redist $1
}

################################################################################
# main function
build_start_time=`$current_date`
case $1 in
    init)
        init_xbus
    ;;
    build)
        build_xbus
    ;;
    clean)
        clean_xbus
    ;;
    test)
        test_xbus
    ;;
    pack)
        if [[ "$2" == "" ]]; then
            echo "\`xbus.sh pack\` need specify a dist folder"
            exit -1
        fi
        pack_xbus $2 $3
    ;;
    help)
        print_help_doc
    ;;
    *)
        echo "bad command \`$1\`, valid command are:"
        print_help_doc
        exit -1
    ;;
esac

build_end_time=`$current_date`
echo "use time $((build_end_time-build_start_time)) ms"
