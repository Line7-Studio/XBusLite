#! /usr/bin/env bash

if [[ $from_python_runtime_pack_main != yes ]]; then
    echo -e "\033[31mYour should run this script from main \033[0m"
    exit 0
fi

# the python packages is argv[1]
var=($(echo $1 | tr ';' '\n'))
py_3rd_packages=("${var[@]}")

# check python
$python_exe --version > /dev/null
if [[ $? != 0 ]]; then
    echo -e "\033[31mYour should have a workable python $python_exe \033[0m"
    exit -1
fi

function create_standard_modules_zip_package()
{

$python_exe <<END

import os
import sys

import pathlib
import zipfile

import py_compile

# change this as your needs
EXCLUDE_FROM_LIBRARY = {
    'site-packages/',
    'pydoc_data/',

    'ensurepip/',

    'turtledemo/',
    'tkinter/',


    'idlelib/',

    'lib2to3/',
    'venv/',

    'distutils/',

    'test/',
}


zip_package_dir = pathlib.Path(os.environ['python_dir'])
lib_root_path = pathlib.Path(os.environ['python_lib_dir'])
compiled_out_dir = pathlib.Path(os.environ['python_runtime_pack_tmp_dir'])

exclude_from_library = [lib_root_path.joinpath(x) for x in EXCLUDE_FROM_LIBRARY]

def iterate_path(path, callback):
    global iterate_path_callback
    for x in path.iterdir():
        if x.is_dir():
            if x not in exclude_from_library:
                if x.name not in ['__pycache__', 'test',]:
                    iterate_path(x, callback)
        else:
            callback(x)


def callback(path):
    if path.suffix != '.py':
        print('skip', path)
        return

    relative_to = path.relative_to(lib_root_path).with_suffix('.pyc')
    compiled_file = compiled_out_dir.joinpath(relative_to)
    print(">>", compiled_file)
    try:
        py_compile.compile(str(path), compiled_file)
    except Exception as e:
        print('compile error', e)
    else:
        zip_file.write(compiled_file, relative_to)

zip_package_file = zip_package_dir.joinpath('lib.zip')

with zipfile.ZipFile(zip_package_file, 'w', zipfile.ZIP_DEFLATED) as zip_file:
    os.makedirs(compiled_out_dir, exist_ok=True)
    iterate_path(lib_root_path, callback)

END

}

# install pip and setuptools
echo "install ensurepip"
$python_exe -m ensurepip > /dev/null
$python_exe -m pip install -U pip > /dev/null
$python_exe -m pip install -U setuptools > /dev/null
$python_exe -m pip list --format=columns > /dev/null

# install required 3rd packages
for one in "${py_3rd_packages[@]}"; do
    echo -e "\033[32mInstall python 3rd party module\033[33m $one \033[0m"
    $python_exe -m pip install -U $one
done

# create zip packaged python standard modules
create_standard_modules_zip_package

# zip required python 3rd module packages
pushd $python_lib_dir/site-packages > /dev/null

find_module_dir=()
find_module_dir_count=0
for dir in $python_lib_dir/site-packages/* ; do
    if [[ ! -d $dir ]]; then
        continue
    fi
    for one in "${py_3rd_packages[@]}"; do
        var=`echo $dir | grep -i '.*'$one'-.*-info'`
        if [[ $var != '' ]]; then
            find_module_dir[$find_module_dir_count]=$var
            find_module_dir_count=$(($find_module_dir_count+1))
        fi
    done
done

all_module_dir=""
for one in "${find_module_dir[@]}"; do
    while read one_line; do
        all_module_dir="$all_module_dir $one_line"
    done < $one/top_level.txt
done

if [[ $all_module_dir != "" ]]; then
    pushd $python_lib_dir/site-packages > /dev/null
        zip -r $python_dir/3rd.zip $all_module_dir -x "*__pycache__*"
    popd > /dev/null
fi

popd > /dev/null


case $os_name in
Linux)
;;
Darwin)
;;
FreeBSD)
;;
Windows)
;;
esac

