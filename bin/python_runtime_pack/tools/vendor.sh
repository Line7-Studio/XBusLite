#! /usr/bin/env bash

if [[ $from_main != yes ]]; then
    echo "Your should run this script from main"
    exit 0
fi

dist=$1

if [[ ! -d $dist ]]; then
    echo "Your should have folder $dist"
    ls $dist
    exit -1
fi

if [[ $os_name == Windows ]]; then
    if [[ $for_64bit ]]; then
        python_dir=$root_dir/dist/$os_name/x64
    else
        python_dir=$root_dir/dist/$os_name/x32
    fi
    python_exe=$python_dir/python.exe
else
    python_dir=$root_dir/dist/$os_name
    python_exe=$python_dir/bin/python3
fi

$python_exe --version

if [[ $? != 0 ]]; then
    echo "Your should have a workable python $python_exe"
    exit -1
fi



if [[ $os_name == Windows ]]; then
    cp $python_dir/python??.dll $dist
    cp $python_dir/python??.zip $dist

    mkdir -p $dist/DLLs
    cp -R $python_dir/DLLs/* $dist/DLLs

    mkdir -p $dist/Lib/site-packages

else
    mkdir -p $dist/bin # bin

    cp $python_dir/bin/python3 $dist/bin/

    mkdir -p $dist/lib # lib
    mkdir -p $dist/lib/python3.6/lib-dynload
    mkdir -p $dist/lib/python3.6/site-packages

    cp $python_dir/lib/python36.zip $dist/lib/ # zip

    cp -R $python_dir/lib/python3.6/lib-dynload/* $dist/lib/python3.6/lib-dynload

fi

