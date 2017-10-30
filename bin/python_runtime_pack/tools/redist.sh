#! /usr/bin/env bash

if [[ $from_python_runtime_pack_main != yes ]]; then
    echo -e "\033[31mYour should run this script from main \033[0m"
    exit 0
fi

# the dist dir is argv[1]
dist=$1

if [[ ! -d $dist ]]; then
    echo "Your should have folder $dist"
    ls $dist
    exit -1
fi

# check python
$python_exe --version > /dev/null
if [[ $? != 0 ]]; then
    echo -e "\033[31mYour should have a workable python $python_exe \033[0m"
    exit -1
fi


if [[ $os_name == Windows ]]; then
    cp $python_dir/python??.dll $dist
    cp $python_dir/python??.exe $dist

    mkdir -p $dist/lib # lib
    cp $python_dir/lib.zip $dist/python36.zip

    mkdir -p $dist/DLLs
    cp -R $python_dir/DLLs/* $dist/DLLs

    mkdir -p $dist/Lib/site-packages
    cp -R $python_dir/lib/python3.6/site-packages/* $dist/Lib/site-packages

else
    mkdir -p $dist/bin # bin
    cp $python_dir/bin/python3 $dist/bin/

    dist_py_lib_dir=$dist/lib/python$py_version_short

    mkdir -p $dist/lib # lib
    cp $python_dir/lib.zip $dist/lib/python36.zip

    mkdir -p $dist_py_lib_dir/lib-dynload
    cp -R $python_lib_dir/lib-dynload/* $dist_py_lib_dir/lib-dynload

    mkdir -p $dist_py_lib_dir/site-packages
    unzip $python_dir/3rd.zip -d $dist_py_lib_dir/site-packages

fi

