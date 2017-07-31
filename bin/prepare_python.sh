#! /usr/bin/env bash

pkg_name=python-3.6.2-macosx10.6.pkg
download_url=https://www.python.org/ftp/python/3.6.2/python-3.6.2-macosx10.6.pkg

if [ ! -f $pkg_name ] ; then
    wget $download_url
fi

if [ ! -d tmp ] ; then
    pkgutil --expand $pkg_name tmp
fi

if [ ! -d python_runtime ] ; then
    mkdir python_runtime
fi

pushd python_runtime > /dev/null

# tar xf ../tmp/Python_Framework.pkg/Payload

find . -type l -delete
find . -type f -name "*.pyc" -delete
find . -type d -name "__pycache__" -delete

# remove unused files
rm -rf Versions/3.6/bin
rm -rf Versions/3.6/etc
rm -rf Versions/3.6/share
rm -rf Versions/3.6/include
rm -rf Versions/3.6/Headers
rm -rf Versions/3.6/Resources
rm -rf Versions/3.6/lib/pkgconfig

# remove unused python modules
rm -rf Versions/3.6/lib/python3.6/config-3.6m-darwin
rm -rf Versions/3.6/lib/python3.6/ensurepip
rm -rf Versions/3.6/lib/python3.6/idlelib
rm -rf Versions/3.6/lib/python3.6/lib2to3
rm -rf Versions/3.6/lib/python3.6/pydoc_data
rm -rf Versions/3.6/lib/python3.6/test
rm -rf Versions/3.6/lib/python3.6/tkinter
rm -rf Versions/3.6/lib/python3.6/turtledemo
rm -rf Versions/3.6/lib/python3.6/venv

# remove unused lib-dynload *.so
pushd Versions/3.6/lib/python3.6/lib-dynload > /dev/null
rm _tkinter.cpython-36m-darwin.so
rm _test*.cpython-36m-darwin.so
popd > /dev/null

popd > /dev/null


################################################################################

pushd python_runtime > /dev/null

# handle *.so linked library rpath
pushd Versions/3.6/lib/python3.6/lib-dynload > /dev/null
for f in *.so
do
    otool -L $f | grep -q '/Library/Frameworks/Python.framework/Versions/'
    if [ ! $? -eq 0 ];then
        continue
    fi
    echo $f
done
popd > /dev/null

popd > /dev/null


