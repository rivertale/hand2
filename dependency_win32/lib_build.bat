@echo off
setlocal

set use_curl=false
set use_tar=false
set all_dependency_exist=true

if not exist %~dp0deps mkdir %~dp0deps
set cmake_dir=%~dp0deps\cmake-3.29.0-rc1-windows-x86_64
set nasm_dir=%~dp0deps\nasm-2.16.01
set perl_dir=%~dp0deps\strawberry-perl-5.38.0.1-64bit-portable
set ssl_dir=%~dp0deps\openssl-3.2.0
set curl_dir=%~dp0deps\curl-8.5.0
set git2_dir=%~dp0deps\libgit2-1.7.2
set cjson_dir=%~dp0deps\cJSON-1.7.17
set cmake_url=https://github.com/Kitware/CMake/releases/download/v3.29.0-rc1/cmake-3.29.0-rc1-windows-x86_64.zip
set nasm_url=https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win64/nasm-2.16.01-win64.zip
set perl_url=https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_5380_5361/strawberry-perl-5.38.0.1-64bit-portable.zip
set ssl_url=https://www.openssl.org/source/openssl-3.2.0.tar.gz
set curl_url=https://curl.se/download/curl-8.5.0.tar.gz
set git2_url=https://github.com/libgit2/libgit2/archive/refs/tags/v1.7.2.zip
set cjson_url=https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.17.tar.gz
set cmake_z_file=cmake-3.29.0-rc1-windows-x86_64.zip
set nasm_z_file=nasm-2.16.01-win64.zip
set perl_z_file=strawberry-perl-5.38.0.1-64bit-portable.zip
set ssl_z_file=openssl-3.2.0.tar.gz
set curl_z_file=curl-8.5.0.tar.gz
set git2_z_file=v1.7.2.zip
set cjson_z_file=v1.7.17.tar.gz

where /q cl.exe
if %ERRORLEVEL% neq 0 (
    echo 'cl' not found - please run under MSVC x64 native tools command prompt
    goto :CLEANUP
)
where /q nmake.exe
if %ERRORLEVEL% neq 0 (
    echo 'nmake' not found - please run under MSVC x64 native tools command prompt
    goto :CLEANUP
)

where /q curl.exe
if  %ERRORLEVEL% equ 0 (
    set use_curl=true
)
where /q tar.exe
if  %ERRORLEVEL% equ 0 (
    set use_tar=true
)

pushd "%~dp0deps"
if %use_curl% == true (
    if not exist "%cmake_dir%" (
        if not exist "%cmake_z_file%" (
            echo Downloading '%cmake_z_file%'
            curl -s -L -O %cmake_url%
        )
    )
    if not exist "%nasm_dir%" (
        if not exist "%nasm_z_file%" (
            echo Downloading '%nasm_z_file%'
            curl -s -L -O %nasm_url%
        )
    )
    if not exist "%perl_dir%" (
        if not exist "%perl_z_file%" (
            echo Downloading '%perl_z_file%'
            curl -s -L -O %perl_url%
        )
    )
    if not exist "%ssl_dir%" (
        if not exist "%ssl_z_file%" (
            echo Downloading '%ssl_z_file%'
            curl -s -L -O %ssl_url%
        )
    )
    if not exist "%curl_dir%" (
        if not exist "%curl_z_file%" (
            echo Downloading '%curl_z_file%'
            curl -s -L -O %curl_url%
        )
    )
    if not exist "%git2_dir%" (
        if not exist "%git2_z_file%" (
            echo Downloading '%git2_z_file%'
            curl -s -L -O %git2_url%
        )
    )
    if not exist "%cjson_dir%" (
        if not exist "%cjson_z_file%" (
            echo Downloading '%cjson_z_file%'
            curl -s -L -O %cjson_url%
        )
    )
)

if %use_tar% == true (
    if not exist "%cmake_dir%" (
        if exist "%cmake_z_file%" (
            echo Extracting '%cmake_z_file%'
            tar -xf "%cmake_z_file%" -C "%~dp0deps" cmake-3.29.0-rc1-windows-x86_64
        )
    )
    if not exist "%nasm_dir%" (
        if exist "%nasm_z_file%" (
            echo Extracting '%nasm_z_file%'
            tar -xf "%nasm_z_file%" -C "%~dp0deps" nasm-2.16.01
        )
    )
    if not exist "%perl_dir%" (
        if exist "%perl_z_file%" (
            mkdir "%~dp0deps\strawberry-perl-5.38.0.1-64bit-portable"
            echo Extracting '%perl_z_file%'
            tar -xf "%perl_z_file%" -C "%~dp0deps\strawberry-perl-5.38.0.1-64bit-portable"
        )
    )
    if not exist "%ssl_dir%" (
        if exist "%ssl_z_file%" (
            echo Extracting '%ssl_z_file%'
            tar -xf "%ssl_z_file%" -C "%~dp0deps" openssl-3.2.0
        )
    )
    if not exist "%curl_dir%" (
        if exist "%curl_z_file%" (
            echo Extracting '%curl_z_file%'
            tar -xf "%curl_z_file%" -C "%~dp0deps" curl-8.5.0
        )
    )
    if not exist "%git2_dir%" (
        if exist "%git2_z_file%" (
            echo Extracting '%git2_z_file%'
            tar -xf "%git2_z_file%" -C "%~dp0deps" libgit2-1.7.2
        )
    )
    if not exist "%cjson_dir%" (
        if exist "%cjson_z_file%" (
            echo Extracting '%cjson_z_file%'
            tar -xf "%cjson_z_file%" -C "%~dp0deps" cJSON-1.7.17
        )
    )
)
popd

if not exist "%cmake_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%cmake_dir%' not found, download and extract it from:
    echo %cmake_url%
    echo.
)
if not exist "%nasm_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%nasm_dir%' not found, download and extract it from:
    echo %nasm_url%
    echo.
)
if not exist "%perl_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%perl_dir%' not found, download and extract the files into the directory from:
    echo %perl_url%
    echo.
)
if not exist "%ssl_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%ssl_dir%' not found, download and extract it from:
    echo %ssl_url%
    echo.
)
if not exist "%curl_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%curl_dir%' not found, download and extract it from:
    echo %curl_url%
    echo.
)
if not exist "%git2_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%git2_dir%' not found, download and extract it from:
    echo %git2_url%
    echo.
)
if not exist "%cjson_dir%" (
    set all_dependency_exist=false
    echo [ERROR] Directory '%cjson_dir%' not found, download and extract it manually from:
    echo %cjson_url%
    echo.
)

if not %all_dependency_exist% == true (
    goto :CLEANUP
)


echo Removing existing library...
if exist "%~dp0..\code\include_win32" (
    del /S /Q "%~dp0..\code\include_win32\*" 1>NUL 2>NUL
    cd "%~dp0..\code\include_win32"
    echo Removed from '%cd%'
)
if exist "%~dp0..\code\lib_win32" (
    del /S /Q "%~dp0..\code\lib_win32\*" 1>NUL 2>NUL
    cd "%~dp0..\code\lib_win32"
    echo Removed from '%cd%'
)

echo Clearing build directory...
rmdir /S /Q "%~dp0deps\ssl" 1>NUL 2>NUL
rmdir /S /Q "%~dp0deps\curl" 1>NUL 2>NUL
rmdir /S /Q "%~dp0deps\git" 1>NUL 2>NUL
rmdir /S /Q "%git2_dir%\build" 1>NUL 2>NUL

echo Clearing existing log...
if not exist "%~dp0log" mkdir "%~dp0log"
del /Q "%~dp0log\*.log" 1>NUL 2>NUL


set PATH=%cmake_dir%\bin;%nasm_dir%;%PATH%
REM OpenSSL
echo Compiling OpenSSL...
setlocal
set LC_ALL=C
set CFLAGS=/nologo /utf-8
cd "%ssl_dir%"                                                              1>"%~dp0log\ssl-1.log" 2>&1 && ^
call "%perl_dir%\portableshell.bat" ./Configure VC-WIN64A --prefix="%~dp0deps\ssl" --openssldir="%~dp0deps\ssl\ssl" --release no-apps no-autoload-config no-docs no-makedepend no-shared no-tests 1>"%~dp0log\ssl-2.log" 2>&1 && ^
nmake clean                                                                 1>"%~dp0log\ssl-3.log" 2>&1 && ^
nmake VERBOSE=1                                                             1>"%~dp0log\ssl-4.log" 2>&1 && ^
nmake install                                                               1>"%~dp0log\ssl-5.log" 2>&1
endlocal

if %ERRORLEVEL% neq 0 (
    echo failed
    goto :CLEANUP
)


REM libcurl
echo Compiling libcurl...
setlocal
set CFLAGS=/nologo /utf-8 /DCURL_CA_FALLBACK=1
cd "%curl_dir%\winbuild"                                                    1>"%~dp0log\curl-1.log" 2>&1 && ^
REM the first clean without previous build will fail
nmake /f Makefile.vc mode=dll clean                                         1>"%~dp0log\curl-2.log" 2>&1
nmake /f Makefile.vc mode=dll WITH_PREFIX="%~dp0deps\curl" WITH_SSL=static SSL_PATH="%~dp0deps\ssl" GEN_PDB=no DEBUG=no MACHINE=x64 RTLIBCFG=static 1>"%~dp0log\curl-3.log" 2>&1
endlocal

if %ERRORLEVEL% neq 0 (
    echo failed
    goto :CLEANUP
)


REM libgit2
echo Compiling libgit2...
( if exist "%git2_dir%\build" rmdir /S /Q "%git2_dir%\build" )              1>"%~dp0log\git-1.log" 2>&1 && ^
mkdir "%git2_dir%\build"                                                    1>"%~dp0log\git-2.log" 2>&1 && ^
cd "%git2_dir%\build"                                                       1>"%~dp0log\git-3.log" 2>&1 && ^
cmake -A x64 -DLIBGIT2_SYSTEM_LIBS=secur32.lib;crypt32.lib -DCMAKE_C_FLAGS="/utf-8 /I \"%~dp0deps\ssl\include\"" -DCMAKE_INSTALL_PREFIX="%~dp0deps\git" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DSTATIC_CRT=ON -DBUILD_TESTS=OFF -DBUILD_CLI=OFF -DUSE_HTTPS="OpenSSL" -DOPENSSL_ROOT_DIR="%~dp0deps\ssl" -DOPENSSL_INCLUDE_DIR="%~dp0deps\ssl\include" %git2_dir% 1>"%~dp0log\git-4.log" 2>&1 && ^
cmake --build %git2_dir%\build --config Release --target install -- -v:n    1>"%~dp0log\git-5.log" 2>&1

if %ERRORLEVEL% neq 0 (
    echo failed
    goto :CLEANUP
)


REM output library
echo Copying library...
if not exist "%~dp0..\code\include_win32\cjson" mkdir "%~dp0..\code\include_win32\cjson"
if not exist "%~dp0..\code\lib_win32" mkdir "%~dp0..\code\lib_win32"
copy /B /Y "%cjson_dir%\cJSON.h" "%~dp0..\code\include_win32\cjson\cJSON.h" 1>NUL
copy /B /Y "%cjson_dir%\cJSON.c" "%~dp0..\code\include_win32\cjson\cJSON.c" 1>NUL
cd "%~dp0..\code\include_win32"
echo Copied to '%cd%'
copy /B /Y "%~dp0deps\curl\bin\libcurl.dll" "%~dp0..\code\lib_win32\libcurl-x64.dll" 1>NUL
copy /B /Y "%~dp0deps\git\bin\git2.dll" "%~dp0..\code\lib_win32\libgit2-x64.dll" 1>NUL
cd "%~dp0..\code\lib_win32"
echo Copied to '%cd%'
echo Done

:CLEANUP
endlocal
