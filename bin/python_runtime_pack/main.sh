#! /usr/bin/env bash

required_python_version=3.6.3
required_python_3rd_packages="PyYAML;NumPy;"

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

var=($(echo $required_python_3rd_packages | tr ';' '\n'))
export py_3rd_packages=("${var[@]}")

print_help_doc()
{
    echo " 1:  build     [build the python from                  ]"
    echo " 2:  clean     [build the python from                  ]"
    echo " 3:  tense     [pack all python runtime                ]"
    echo " 4:  copy_to   [pack all python runtime                ]"
    echo " 5:  help      [print help document                    ]"
    exit 0
}

# jump in script located dir
root_dir=`dirname $0`
pushd $root_dir > /dev/null
export root_dir=`pwd`
export from_main=yes

case $1 in
    build)
        ./tools/build.sh
    ;;
    tense)
        ./tools/tense.sh
    ;;
    vendor)
        ./tools/vendor.sh $2
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
        echo "bad command! valid command are:"
        print_help_doc
        exit -1
    ;;
esac

# jump out script located dir
popd > /dev/null
