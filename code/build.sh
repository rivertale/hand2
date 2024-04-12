#!/bin/sh

program_name="hand"
linux_entry_file="../code/linux_main.c"

musl_obj_files="../code/lib_linux/Scrt1.o ../code/lib_linux/crti.o ../code/lib_linux/crtn.o"
musl_compiler_flags="-nostdinc -isystem ../code/include_linux/musl"
musl_linker_flags="-nostdlib -lc"

gcc_compiler_flags="-Wall -g -I ../code/include_linux ${musl_compiler_flags}"
gcc_linker_flags="-static -L ../code/lib_linux -lgit2 -lcurl -lssl -lcrypto ${musl_linker_flags}"
clang_compiler_flags="-Wall -g -I ../code/include_linux ${musl_compiler_flags}"
clang_linker_flags="-static -L ../code/lib_linux -lgit2 -lcurl -lssl -lcrypto ${musl_linker_flags}"

mkdir -p ../build
cd ../build
cp -f -t ./ ../code/lib/*.o 2>/dev/null
cp -f -t ./ ../code/lib_linux/*.o 2>/dev/null

# choose compiler
compiler="$1"
gcc_enabled=0
clang_enabled=0
gcc_suffix=""
clang_suffix=""
if [ -z ${compiler} ]; then compiler="any"; fi

which gcc 1>/dev/null 2>/dev/null
if [ $? -eq 0 ]
then
    if [ ${compiler} = "any" ]; then compiler="gcc"; fi
fi
which clang 1>/dev/null 2>/dev/null
if [ $? -eq 0 ]
then
    if [ ${compiler} = "any" ]; then compiler="clang"; fi
fi

if [ ${compiler} = "gcc" ]
then
    gcc_enabled=1
elif [ ${compiler} = "clang" ]
then
    clang_enabled=1
elif [ ${compiler} = "all" ]
then
    gcc_enabled=1
    gcc_suffix="_gcc"
    clang_enabled=1
    clang_suffix="_clang"
elif [ ${compiler} = "any" ]
then
    echo "[no compiler found]"
    echo "Available compilers are: any, all, msvc, gcc, clang"
    echo "Default compiler is: any"
    echo "\n"
else
    echo "[unknown compiler ${compiler}]"
    echo "Available compilers are: any, all, msvc, gcc, clang"
    echo "Default compiler is: any"
    echo "\n"
fi

# gcc
if [ ${gcc_enabled} -ne 0 ]
then
    echo "[compiling with gcc]"
    which gcc 1>/dev/null 2>/dev/null
    if [ $? -ne 0 ]
    then
        echo "GCC aborted: 'gcc' not found"
    else
        gcc ${gcc_compiler_flags} -o ${program_name}${gcc_suffix} ${linux_entry_file} ${musl_obj_files} ${gcc_linker_flags}
        if [ $? -eq 0 ]; then echo "Output binary: ${program_name}${gcc_suffix}"; fi
    fi
    echo "\n"
fi

# clang
if [ ${clang_enabled} -ne 0 ]
then
    echo "[compiling with clang]"
    which clang 1>/dev/null 2>/dev/null
    if [ $? -ne 0 ]
    then
        echo "clang aborted: 'clang' not found"
    else
        clang ${clang_compiler_flags} -o ${program_name}${clang_suffix} ${linux_entry_file} ${musl_obj_files} ${clang_linker_flags}
        if [ $? -eq 0 ]; then echo "Output binary: ${program_name}${clang_suffix}"; fi
    fi
    echo "\n"
fi

# docker script
echo "[copying docker script]"
mkdir -p ./docker
cp -v -f -t ./docker ../docker/Dockerfile 2>/dev/null
cp -v -f -t ./docker ../docker/run.sh 2>/dev/null
echo "\n"

rm -f ./*.o 2>/dev/null
