#! /usr/bin/env bash

if [[ $from_main != yes ]]; then
    echo "Your should run this script from main"
    exit 0
fi

echo "start building python $os_name"

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

    pushd PCbuild > /dev/null
        cmd //c build.bat -e --no-tkinter
        cmd //c build.bat -e --no-tkinter -p x64
    popd > /dev/null

    dist_x32=$root_dir/dist/$os_name/x32
    dist_x64=$root_dir/dist/$os_name/x64

    function build_python()
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

    python_exe=$dist_x32/python.exe
    $python_exe --version
    if [[ $? == 0 ]]; then
        echo "Seems we alreadly have a working x32 python at $python_exe"
    else
        echo "Build python x32 runtimes ..."
        pushd PCbuild/win32 > /dev/null
            prefix=$dist_x32; build_python
        popd > /dev/null
    fi

    python_exe=$dist_x64/python.exe
    $python_exe --version
    if [[ $? == 0 ]]; then
        echo "Seems we alreadly have a working x64 python at $python_exe"
    else
        echo "Build python x64 runtimes ..."
        pushd PCbuild/amd64 > /dev/null
            prefix=$dist_x64; build_python
        popd > /dev/null
    fi

else
    # must be absoult path, so we need root_dir here
    prefix=$root_dir/dist/$os_name

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
        ./configure --prefix=$prefix $extra_flags && make -j8 && make -j8 install
        # TODO:
        pushd $prefix/lib/python$py_version_short/lib-dynload > /dev/null
            rm _test*
            rm _ctypes_test*
        popd > /dev/null
    }
    python_exe=$root_dir/dist/$os_name/bin/python3
    $python_exe --version
    if [[ $? == 0 ]]; then
        echo "Seems we alreadly have a working python at $python_exe"
    else
        echo "Build python runtimes ..."
        build_python
    fi

fi

popd > /dev/null
#################################### build #####################################

