@echo off

if not exist ..\release mkdir ..\release
pushd ..\release

REM usage
set abort_archive=0
set version=%1
if "%version%" == "" (
    echo usage: release.bat version
    goto ABORT
)

REM prepare files
if exist hand2 rmdir /s /q hand2
del /f hand2-v%version%.zip 2>nul
mkdir hand2
copy ..\build\hand.exe hand2 1>nul
if %ERRORLEVEL% neq 0 (
    set abort_archive=1
    echo Can't find ..\build\hand.exe
)
copy ..\build\libcurl-x64.dll hand2 1>nul
if %ERRORLEVEL% neq 0 (
    set abort_archive=1
    echo Can't find ..\build\libcurl-x64.dll
)
copy ..\build\libgit2-x64.dll hand2 1>nul
if %ERRORLEVEL% neq 0 (
    set abort_archive=1
    echo Can't find ..\build\libgit2-x64.dll
)
copy ..\build\hand hand2 1>nul
if %ERRORLEVEL% neq 0 (
    set abort_archive=1
    echo Can't find ..\build\hand
)
if %abort_archive% neq 0 goto ABORT

REM archive
tar -cf hand2-v%version%.zip hand2
if %ERRORLEVEL% equ 0 echo Output file: hand2-v%version%.zip

:ABORT
rmdir /s /q hand2
popd
