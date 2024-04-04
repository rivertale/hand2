@echo off
setlocal

set program_name=hand
set win32_entry_file=..\code\win32_main.c

set msvc_compiler_flags=/nologo /Zi /MT /Wall /wd4668 /wd4710 /wd4996 /wd5045 /I ..\code\include_win32
set msvc_linker_flags=/IGNORE:4217,4049 /incremental:no /LIBPATH:..\code\lib_win32
set gcc_compiler_flags=-Wall -I ..\code\include_win32
set gcc_linker_flags=-L ..\code\lib_win32
set clang_compiler_flags=-Wall -I ..\code\include
set clang_linker_flags=-L ..\code\lib_win32

if not exist ..\build mkdir ..\build
pushd ..\build

echo [compiling with msvc]
where /q cl.exe
if %ERRORLEVEL% neq 0 (
    echo MSVC aborted: 'cl' not found - please run under MSVC x64 native tools command prompt
) else (
    cl %msvc_compiler_flags% %win32_entry_file% /link /out:%program_name%_msvc.exe %msvc_linker_flags%
    if %ERRORLEVEL% equ 0 (echo Output binary: %program_name%_msvc.exe)
)
echo.

echo [compiling with gcc]
where /q gcc
if %ERRORLEVEL% neq 0 (
    echo GCC aborted: 'gcc' not found
) else (
    gcc %gcc_compiler_flags% -o %program_name%_gcc.exe %win32_entry_file% %gcc_linker_flags%
    if %ERRORLEVEL% equ 0 (echo Output binary: %program_name%_gcc.exe)
)
echo.

echo [compiling with clang]
where /q clang
if %ERRORLEVEL% neq 0 (
    echo clang aborted: 'clang' not found
) else (
    clang %clang_compiler_flags% -o %program_name%_clang.exe %clang_linker_flags% %win32_entry_file%
    if %ERRORLEVEL% equ 0 (echo Output binary: %program_name%_clang.exe)
)
echo.

echo [copying library]
copy ..\code\lib_win32\*.dll .\ 2>nul
echo.

del /q *.obj *.lib *.exp vc140.pdb 2>nul
popd

endlocal