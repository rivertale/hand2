#!/bin/sh
cd "$(dirname "$0")"
base_dir="$(pwd "$0")"
image_name="hand2_external_linux"
include_dir="hand/code/include_linux"
library_dir="hand/code/lib_linux"
log_dir="hand/linux_external/log"

# build docker
cd "${base_dir}"
docker build --no-cache -t "${image_name}" .
if [ $? -eq 0 ]; then echo "Output docker image: ${image_name}"; fi

# build external library
cd "${base_dir}"
cd ..
docker run --rm --user $(id -u):$(id -g) --volume "$(pwd):/hand" "${image_name}" sh -c "
    echo \"[Copying enviroment logs]\"
    mkdir -p /${log_dir}
    rm -f /${log_dir}/*.log
    cp -r /log/*.log /${log_dir}
    echo \"Copied to '${log_dir}'\"
    echo \"\"

    echo \"[Removing existing library]\"
    rm -rf /${include_dir}/*
    rmdir /${include_dir}
    echo \"Removed from '${include_dir}'\"
    rm -f /${library_dir}/*
    rmdir /${library_dir}
    echo \"Removed from '${library_dir}'\"
    echo \"\"

    echo \"[Copying libraries]\"
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

echo "[Removing docker image]"
docker image rm "${image_name}"
