@echo OFF
setlocal

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: check needed tool set ok

where /q ninja
if ERRORLEVEL 1 (
    echo command ninja does not find.
    echo make sure ninja is installed and placed in path.
    exit /B
)

where /q cl
if ERRORLEVEL 1 (
    echo command msvc cl does not find.
    echo make sure msvc cl is installed and placed in path.
    exit /B
)

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
setlocal enabledelayedexpansion
:: trace build time
set build_start_time=%time%

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%1"=="init" goto INIT
if "%1"=="build" goto BUILD
if "%1"=="clean" goto CLEAN
if "%1"=="test" goto TEST
if "%1"=="pack" goto PACK

if "%1"=="GEN_CLIENT_HOST_SRC" goto GEN_CLIENT_HOST_SRC

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo bad command! valid command are:
echo  0:  init       [generate xbus project depend files  ]
echo  1:  build      [build the application               ]
echo  2:  clean      [clean generated files               ]
echo  3:  test       [run all unit test                   ]
echo  4:  pack       [pack xbus framework                 ]
goto :END


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:INIT

mkdir tmp 2>nul
goto :END


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:BUILD

for /f "delims== tokens=1,2" %%G in (xbus.xbus) do (
    set %%G=%%H
)

set CC=cl
set CXX=cl
set LD=link

set SUPPORT_WINDOWS_XP=True

if /I %SUPPORT_WINDOWS_XP%==true (
    set LIB=C:\Program Files ^(x86^)\Microsoft SDKs\Windows\v7.1A\lib;!LIB!
    set LIBPATH=C:\Program Files ^(x86^)\Microsoft SDKs\Windows\v7.1A\lib;!LIBPATH!
    set INCLUDE=C:\Program Files ^(x86^)\Microsoft SDKs\Windows\v7.1A\include;!INCLUDE!
)

set cmake_args=-G Ninja

if /I %xbus_build_type%==release (
    set cmake_args=!cmake_args! -DCMAKE_BUILD_TYPE=Release
) else (
    set cmake_args=!cmake_args! -DCMAKE_BUILD_TYPE=Debug
)

if /I %SUPPORT_WINDOWS_XP%==true (
    set cmake_args=!cmake_args! -DSUPPORT_WINDOWS_XP=True
)

echo generate cmake files
mkdir tmp\build 2>nul
pushd tmp\build
cmake %cmake_args% ../..
popd

pushd tmp\build
ninja

if ERRORLEVEL 1 (
    echo build xbus project failed
    popd && goto :END
) else (
    popd
)

goto :END

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:CLEAN
if exist tmp\build rmdir /s /q tmp\build
goto :END


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:TEST
pushd tmp\build
ctest -V
popd
goto :END


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:PACK

goto :END


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:END
set build_end_time=%time%
echo use time %build_start_time% -- %build_end_time%
endlocal

