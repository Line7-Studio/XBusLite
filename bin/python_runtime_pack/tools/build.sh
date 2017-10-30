#! /usr/bin/env bash

if [[ $from_python_runtime_pack_main != yes ]]; then
    echo -e "\033[31mYour should run this script from main \033[0m"
    exit 0
fi

echo -e "\033[32mStart building python for $os_name ... \033[0m"

mkdir -p build
mkdir -p dist/$os_name

################################### download ###################################
base_url=https://www.python.org/ftp/python
if [[ ! -f build/python.tar.xz  ]]; then
  curl -o build/python.tar.xz $base_url/$py_version_full/Python-$py_version_full.tar.xz
fi

#################################### unzip #####################################
mkdir -p build/$os_name > /dev/null
if [[ ! -d build/$os_name/Python-$py_version_full ]]; then
   tar  xf build/python.tar.xz -C build/$os_name
fi

#################################### build #####################################
pushd build/$os_name/Python-$py_version_full > /dev/null

if [[ $os_name == Windows ]]; then

    function copy_built_python_to_dist()
    {
        mkdir -p $prefix
        cp python.exe $prefix
        cp python*.dll $prefix

        mkdir -p $prefix/libs
        cp python3.lib $prefix/libs
        cp python36.lib $prefix/libs

        mkdir -p $prefix/DLLs
        cp sqlite3.dll $prefix/DLLs
        cp *.pyd $prefix/DLLs
        pushd $prefix/DLLs > /dev/null
            rm _test*.pyd
            rm _msi.pyd
            rm _ctypes_test.pyd
        popd > /dev/null

        mkdir -p $prefix/Include
        cp ../../Include/*.h $prefix/Include
        cp ../../PC/pyconfig.h $prefix/Include

        mkdir -p $prefix/Lib
        cp -rf ../../Lib/* $prefix/Lib
    }

    $python_exe --version > /dev/null
    if [[ $? == 0 ]]; then
        echo "Seems we alreadly have a working python at $python_exe"
    else
        echo "Build python x32 runtimes ..."
        pushd PCbuild > /dev/null
            cmd //c build.bat -e --no-tkinter
        popd > /dev/null
        pushd PCbuild/win32 > /dev/null
            dist_x32=$python_dir/../x32
            prefix=$dist_x32
            copy_built_python_to_dist
        popd > /dev/null

        echo "Build python x64 runtimes ..."
        pushd PCbuild > /dev/null
            cmd //c build.bat -e --no-tkinter -p x64
        popd > /dev/null
        pushd PCbuild/amd64 > /dev/null
            dist_x64=$python_dir/../x64
            prefix=$dist_x64
            copy_built_python_to_dist
        popd > /dev/null
    fi

else
    extra_flags=""
    extra_flags="$extra_flags --enable-ipv6"
    extra_flags="$extra_flags --without-ensurepip"

    if [[ $os_name == macOS ]]; then
        # TODO:
        extra_flags="$extra_flags LDFLAGS=-L/usr/local/opt/openssl/lib"
        extra_flags="$extra_flags CPPFLAGS=-I/usr/local/opt/openssl/include"
    else
        # TODO:
        echo "TODO: handle configure flags for $os_name"
    fi

    function build_python()
    {
        ./configure --prefix=$python_dir $extra_flags && make -j8 && make -j8 install
        # TODO:
        pushd $python_lib_dir/lib-dynload > /dev/null
            rm _test*
            rm _ctypes_test*
        popd > /dev/null
    }
    $python_exe --version > /dev/null
    if [[ $? == 0 ]]; then
        echo "Seems we alreadly have a working python at $python_exe"
    else
        echo "Build python runtimes ..."
        build_python
    fi

fi

popd > /dev/null
#################################### build #####################################

