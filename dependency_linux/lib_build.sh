#!/bin/sh
image_name="hand2_linux_image"
include_dir="hand/code/include_linux"
library_dir="hand/code/lib_linux"
log_dir="hand/dependency_linux/log"

cd $(dirname $0)
docker build -t ${image_name} .
if [ $? -eq 0 ]; then echo "Output docker image: ${image_name}"; fi

cd $(dirname $0)
cd ..
docker run --rm -v $(pwd):/hand ${image_name} sh -c "
    echo \"[Copying enviroment logs]\"
    mkdir -p /${log_dir}
    rm -f /${log_dir}/*.log
    cp -r /log/*.log /${log_dir}
    echo \"Copied to '${log_dir}'\"
    echo \"\"

    echo \"[Removing existing library]\"
    rm -rf /${include_dir}/*
    echo \"Removed from '${include_dir}'\"
    rm -f /${library_dir}/*
    echo \"Removed from '${library_dir}'\"
    echo \"\"

    echo \"[Checking host system case sensitivity]\"
    mkdir -p /${include_dir}
    rm -f /${include_dir}/A
    touch /${include_dir}/a
    if test -e /${include_dir}/A
    then
        echo \"Case sensitivity: off\"
        echo \"WARNING: Copied library might be incomplete (should still work)\"
    else
        echo \"Case sensitivity: on\"
    fi
    rm -f /${include_dir}/a
    rm -f /${include_dir}/A
    echo \"\"

    echo \"[Copying libraries]\"
    mkdir -p /${include_dir}/musl
    mkdir -p /${include_dir}/cjson
    cp -rf /deps/musl/include/*         /${include_dir}/musl/
    cp -f /deps/cJSON-1.7.17/cJSON.h    /${include_dir}/cjson/cJSON.h
    cp -f /deps/cJSON-1.7.17/cJSON.c    /${include_dir}/cjson/cJSON.c
    echo \"Copied to '${include_dir}'\"
    mkdir -p /${library_dir}
    cp -f /deps/musl/lib/Scrt1.o        /${library_dir}/
    cp -f /deps/musl/lib/crti.o         /${library_dir}/
    cp -f /deps/musl/lib/crtn.o         /${library_dir}/
    cp -f /deps/musl/lib/libc.a         /${library_dir}/
    cp -f /deps/ssl/lib/libcrypto.a     /${library_dir}/
    cp -f /deps/ssl/lib/libssl.a        /${library_dir}/
    cp -f /deps/curl/lib/libcurl.a      /${library_dir}/
    cp -f /deps/git/libgit2.a           /${library_dir}/
    echo \"Copied to '${library_dir}'\"
    echo \"\"
"