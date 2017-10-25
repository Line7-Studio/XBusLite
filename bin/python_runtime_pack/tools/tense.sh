#! /usr/bin/env bash

if [[ $from_main != yes ]]; then
    echo -e "\033[31mYour should run this script from main \033[0m"
    exit 0
fi


# locate which python
if [[ $os_name == Windows ]]; then
    if [[ $for_64bit ]]; then
        python_exe=$root_dir/dist/$os_name/x64/python.exe
    else
        python_exe=$root_dir/dist/$os_name/x32/python.exe
    fi
else
    python_exe=$root_dir/dist/$os_name/bin/python3
fi

function create_zip_package()
{

$python_exe <<END

import os
import sys

import pathlib
import zipfile

import py_compile


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

COMPILED_OUT_DIR='tmp/'
BASE_NAME = 'python{0.major}{0.minor}'.format(sys.version_info)

if sys.platform == 'win32':
    lib_root_path = pathlib.Path(sys.exec_prefix + '/Lib')
    zip_package_dir = pathlib.Path(sys.exec_prefix)
else:
    version_name = 'python{0.major}.{0.minor}'.format(sys.version_info)
    lib_root_path = pathlib.Path(sys.exec_prefix + '/lib/'+version_name)
    zip_package_dir = pathlib.Path(sys.exec_prefix + '/lib')


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
    compiled_file = COMPILED_OUT_DIR + str(relative_to)
    print(">>>>>>>>>>>>>> ", compiled_file)
    try:
        py_compile.compile(str(path), compiled_file)
    except Exception as e:
        print('compile error', e)
    else:
        zip_file.write(compiled_file, relative_to)


zip_package_file = zip_package_dir.joinpath(BASE_NAME + '.zip')

with zipfile.ZipFile(zip_package_file, 'w', zipfile.ZIP_DEFLATED) as zip_file:
    os.makedirs(COMPILED_OUT_DIR, exist_ok=True)
    iterate_path(lib_root_path, callback)

END

}


# install pip and setuptools
$python_exe --version
# $python_exe -m ensurepip > /dev/null
# $python_exe -m pip install -U pip > /dev/null
# $python_exe -m pip install -U setuptools > /dev/null
# $python_exe -m pip list --format=columns > /dev/null

# install required 3rd packages
for one in "${py_3rd_packages[@]}"; do
    # $python_exe -m pip install -U $one
    echo "$one"
done

$python_exe -m pip list --format=columns

# create zip packaged python standard modules

module_location=`$python_exe -c "import os;print(os.__file__);" `

if [[ $os_name == Windows ]]; then
    echo "$module_location" | grep '^.*python..\.zip\\os\.pyc$' > /dev/null
else
    echo "$module_location" | grep '^.*python..\.zip/os\.pyc$' > /dev/null
fi

if [[ $? == 0 ]]; then
    echo "zip packaged standard modules ok"
else
    create_zip_package
fi

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

