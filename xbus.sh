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

################################################################################
print_help_doc()
{
    echo " 0:  init      [generate xbus project depend files     ]"
    echo " 1:  build     [build the application                  ]"
    echo " 2:  clean     [clean generated files                  ]"
    echo " 3:  test      [run all unit tests                     ]"
    echo " 4:  pack      [pack xbus framework                    ]"
    echo " 5:  help      [print help document                    ]"
    exit 0
}

init_xbus()
{
    bin/python_runtime_pack/main.sh build
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

## TODO: ???
pack_xbus()
{
    pushd $build_dir >/dev/null
    popd > /dev/null
}

################################################################################
# main function
build_start_time=`$current_date`

case $@ in
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
        pack_xbus
    ;;
    help)
        print_help_doc
    ;;
    *)
        echo "bad command! valid command are:"
        print_help_doc
        exit -1
    ;;
esac

build_end_time=`$current_date`
echo "use time $((build_end_time-build_start_time)) ms"
