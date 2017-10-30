#! /usr/bin/env bash

required_python_version=3.6.3

case `uname` in
    Linux)
        export os_name=Linux
        ;;
    Darwin)
        export os_name=macOS
        ;;
    FreeBSD)
        export os_name=FreeBSD
        ;;
    *_NT-*)
        export os_name=Windows
        hash cl &> /dev/null
        if [ $? -eq 0 ] ; then
            if [[ `cl 2>&1` =~ .*x64.* ]]; then
                export for_64bit=true
            fi
        else
            echo "Need Set MSVC First On Windows"
            exit -1
        fi
        ;;
    *)
        echo "None Supported OS"
        exit -1
        ;;
esac

var=($(echo $required_python_version | tr '.' '\n'))
export py_version_full="${var[0]}.${var[1]}.${var[2]}"
export py_version_short="${var[0]}.${var[1]}"
export py_version_nodot="${var[0]}${var[1]}"

print_help_doc()
{
    echo -e " 1: build  :\033[32m build python $required_python_version \033[0m"
    echo -e " 2: clean  :\033[32m remove all used tmp files             \033[0m"
    echo -e " 3: tense  :\033[32m pack and zip built python runtime     \033[0m"
    echo -e " 4: redist :\033[32m copy built python runtime to somewhere\033[0m"
    echo -e " 5: help   :\033[32m print help document                   \033[0m"
    exit 0
}

# jump in script located dir
root_dir=`dirname $0`
pushd $root_dir > /dev/null
root_dir=`pwd`


# need this on Windows
if [[ $os_name == Windows ]]; then
    root_dir=`cygpath -m $root_dir`
fi

export from_python_runtime_pack_main=yes

# export needed path info
if [[ $os_name == Windows ]]; then
    if [[ $for_64bit ]]; then
        export python_dir=$root_dir/dist/$os_name/x64
        export python_runtime_pack_tmp_dir=$root_dir/tmp/$os_name/x64
    else
        export python_dir=$root_dir/dist/$os_name/x32
        export python_runtime_pack_tmp_dir=$root_dir/tmp/$os_name/x32
    fi
    export python_exe=$python_dir/python.exe
    export python_lib_dir=$python_dir/Lib
else
    export python_dir=$root_dir/dist/$os_name
    export python_runtime_pack_tmp_dir=$root_dir/tmp/$os_name
    export python_exe=$python_dir/bin/python3
    export python_lib_dir=$python_dir/lib/python$py_version_short
fi

# dispatch as argv[1]
case $1 in
    build)
        ./tools/build.sh
    ;;
    tense)
        ./tools/tense.sh $2
    ;;
    redist)
        ./tools/redist.sh $2
    ;;
    clean)
        rm -rf build/
        rm -rf dist/
        rm -rf tmp/
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

# jump out script located dir
popd > /dev/null
