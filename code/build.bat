@echo off
setlocal

set program_name=hand
set win32_entry_file=..\code\win32_main.c

set msvc_compiler_flags=/nologo /Zi /MT /Wall /wd4255 /wd4668 /wd4710 /wd4820 /wd4996 /wd5045 /I ..\code\include_win32
set msvc_linker_flags=/IGNORE:4217,4049 /incremental:no /LIBPATH:..\code\lib_win32
set gcc_compiler_flags=-Wall -g -I ..\code\include_win32
set gcc_linker_flags=-static -L ..\code\lib_win32
set clang_compiler_flags=-Wall -Wno-deprecated-declarations -g -I ..\code\include_win32
set clang_linker_flags=-static -L ..\code\lib_win32

if not exist ..\build mkdir ..\build
pushd ..\build

REM choose compiler
set compiler=%1
set msvc_enabled=0
set msvc_suffix=
set gcc_enabled=0
set gcc_suffix=
set clang_enabled=0
set clang_suffix=
if "%compiler%" == "" (
    set compiler=any
)
where /q cl
if %ERRORLEVEL% equ 0 (
    if "%compiler%" == "any" set compiler=msvc
)
where /q gcc
if %ERRORLEVEL% equ 0 (
    if "%compiler%" == "any" set compiler=gcc
)
where /q clang
if %ERRORLEVEL% equ 0 (
    if "%compiler%" == "any" set compiler=clang
)

if "%compiler%" == "msvc" (
    set msvc_enabled=1
) else if "%compiler%" == "gcc" (
    set gcc_enabled=1
) else if "%compiler%" == "clang" (
    set clang_enabled=1
) else if "%compiler%" == "all" (
    set msvc_enabled=1
    set msvc_suffix=_msvc
    set gcc_enabled=1
    set gcc_suffix=_gcc
    set clang_enabled=1
    set clang_suffix=_clang
) else if "%compiler%" == "any" (
    echo [no compiler found]
    echo Available compilers are: any, all, msvc, gcc, clang
    echo Default compiler is: any
    echo.
) else (
    echo [unknown compiler %compiler%]
    echo Available compilers are: any, all, msvc, gcc, clang
    echo Default compiler is: any
    echo.
)

REM msvc
if %msvc_enabled% equ 0 goto MSVC_END
echo [compiling with msvc]
where /q cl
if %ERRORLEVEL% neq 0 (
    echo MSVC aborted: 'cl' not found - please run under MSVC x64 native tools command prompt
    goto MSVC_END
)
cl %msvc_compiler_flags% %win32_entry_file% /link /out:%program_name%%msvc_suffix%.exe %msvc_linker_flags%
if %ERRORLEVEL% equ 0 echo Output binary: %program_name%%msvc_suffix%.exe
echo.
:MSVC_END

REM gcc
if %gcc_enabled% equ 0 goto GCC_END
echo [compiling with gcc]
where /q gcc
if %ERRORLEVEL% neq 0 (
    echo GCC aborted: 'gcc' not found
    echo.
    goto GCC_END
)
gcc %gcc_compiler_flags% -o %program_name%%gcc_suffix%.exe %win32_entry_file% %gcc_linker_flags%
if %ERRORLEVEL% equ 0 echo Output binary: %program_name%%gcc_suffix%.exe
echo.
:GCC_END

REM clang
if %clang_enabled% equ 0 goto CLANG_END
echo [compiling with clang]
where /q clang
if %ERRORLEVEL% neq 0 (
    echo clang aborted: 'clang' not found
    goto CLANG_END
)
clang %clang_compiler_flags% -o %program_name%%clang_suffix%.exe %clang_linker_flags% %win32_entry_file%
if %ERRORLEVEL% equ 0 echo Output binary: %program_name%%clang_suffix%.exe
echo.
:CLANG_END

REM library
echo [copying library]
copy ..\code\lib_win32\*.dll .\ 2>nul
echo.

del /q *.obj *.lib *.ilk *.exp vc140.pdb 2>nul
popd

endlocal
