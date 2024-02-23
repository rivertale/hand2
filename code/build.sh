#!/bin/sh
mkdir -p ../build
cd ../build
cp -f -t ./ ../code/lib/*.o 2>/dev/null
cp -f -t ./ ../code/lib_linux/*.o 2>/dev/null

program_name="hand"
linux_entry_file="../code/linux_main.c"

musl_obj_files="../code/lib_linux/Scrt1.o ../code/lib_linux/crti.o ../code/lib_linux/crtn.o"
musl_compiler_flags="-nostdinc -isystem ../code/include_linux/musl"
musl_linker_flags="-nostdlib -lc"

linux_obj_files="${musl_obj_files}"
gcc_compiler_flags="-Wall -g -I ../code/include_linux ${musl_compiler_flags}"
gcc_linker_flags="-static -L ../code/lib_linux -lgit2 -lcurl -lssl -lcrypto ${musl_linker_flags}"
clang_compiler_flags="-Wall -g -I ../code/include_linux ${musl_compiler_flags}"
clang_linker_flags="-static -L ../code/lib_linux -lgit2 -lcurl -lssl -lcrypto ${musl_linker_flags}"

echo "[compiling with gcc]"
which gcc 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]
then
    echo "GCC aborted: 'gcc' not found"
else
    gcc ${gcc_compiler_flags} -o ${program_name}_gcc ${linux_entry_file} ${linux_obj_files} ${gcc_linker_flags}
    if [ $? -eq 0 ]; then echo "Output binary: ${program_name}_gcc"; fi
fi
echo "\n"

echo "[compiling with clang]"
which clang 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]
then
    echo "clang aborted: 'clang' not found"
else
    clang ${clang_compiler_flags} -o ${program_name}_clang ${linux_entry_file} ${linux_obj_files} ${clang_linker_flags}
    if [ $? -eq 0 ]; then echo "Output binary: ${program_name}_clang"; fi
fi
echo "\n"

echo "[copying data]"
cp -v -f -t ./ ../data/* 2>/dev/null
echo "\n"

rm -f ./*.o 2>/dev/null
