#! /usr/bin/env bash

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
    echo ''
    git submodule init
    git submodule update
}

build_xbus()
{
    # load xbus.xbus
    while read line; do
        eval $line
    done < xbus.xbus

    cmake_args="-G Ninja"
    if [ "$xbus_build_type" == "release" ]; then
        cmake_args="$cmake_args -DCMAKE_BUILD_TYPE=Release"
    else
        cmake_args="$cmake_args -DCMAKE_BUILD_TYPE=Debug"
    fi

    echo "generate cmake files"
    mkdir -p tmp/build
    pushd tmp/build >/dev/null
    cmake $cmake_args ../..
    popd >/dev/null

    echo "ninja build"
    pushd tmp/build > /dev/null
    ninja
    popd > /dev/null
}

clean_xbus()
{
    rm -rf tmp/build
    rm -rf dst/*
}

test_xbus()
{
    pushd tmp/build > /dev/null
    ctest -V
    popd > /dev/null
}

pack_xbus()
{
    pushd tmp/build > /dev/null
    popd > /dev/null
}

# main function

for var in "$@"
    do
    case $var in
    init)
        init_xbus
    exit 0
    ;;
    build)
        build_xbus
    exit 0
    ;;
    clean)
        clean_xbus
    exit 0
    ;;
    test)
        test_xbus
    exit 0
    ;;
    pack)
        pack_xbus
    exit 0
    ;;
    help)
        print_help_doc
    exit 0
    ;;
    GEN_CLIENT_HOST_SRC)
        gen_client_host_src $2
    exit 0
    ;;
    esac
done

echo "bad command! valid command are:"
print_help_doc
