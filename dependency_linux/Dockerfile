FROM ubuntu:22.04
ARG my_c_compiler="/deps/musl/bin/musl-gcc"

# env
RUN mkdir /deps && \
    mkdir /log && \
    apt-get update && \
    apt-get install -y clang cmake gcc gdb make nasm perl tar unzip wget xz-utils

# version
RUN clang --version 1>/log/env-clang.log 2>&1 && \
    cmake --version 1>/log/env-cmake.log 2>&1 && \
    gcc   --version 1>/log/env-gcc.log 2>&1 && \
    gdb   --version 1>/log/env-gdb.log 2>&1 && \
    make  --version 1>/log/env-make.log 2>&1 && \
    nasm  --version 1>/log/env-nasm.log 2>&1 && \
    perl  --version 1>/log/env-perl.log 2>&1 && \
    tar   --version 1>/log/env-tar.log 2>&1 && \
    unzip --help    1>/log/env-unzip.log 2>&1 && \
    wget  --version 1>/log/env-wget.log 2>&1 && \
    xz    --version 1>/log/env-xz.log 2>&1

# musl
RUN cd /deps                                                                    1>/log/musl-1.log 2>&1 && \
    wget https://musl.libc.org/releases/musl-1.2.4.tar.gz                       1>/log/musl-2.log 2>&1 && \
    tar -xf musl-1.2.4.tar.gz                                                   1>/log/musl-3.log 2>&1 && \
    cd musl-1.2.4                                                               1>/log/musl-4.log 2>&1 && \
    ./configure --prefix="/deps/musl" --target=x86_64-linux-gnu --enable-optimize --disable-debug --enable-shared --enable-static \
                                                                                1>/log/musl-5.log 2>&1 && \
    V=1 make -j "$(nproc)"                                                      1>/log/musl-6.log 2>&1 && \
    V=1 make -j "$(nproc)" install                                              1>/log/musl-7.log 2>&1

# linux-kernel-header
RUN cd /deps                                                                    1>/log/hdr-1.log 2>&1 && \
    wget https://github.com/sabotage-linux/kernel-headers/releases/download/v4.19.88-2/linux-headers-4.19.88-2.tar.xz \
                                                                                1>/log/hdr-2.log 2>&1 && \
    tar -xf linux-headers-4.19.88-2.tar.xz                                      1>/log/hdr-3.log 2>&1 && \
    cd linux-headers-4.19.88-2                                                  1>/log/hdr-4.log 2>&1 && \
    V=1 make ARCH=x86_64 DESTDIR="/deps/musl" prefix="" install                 1>/log/hdr-5.log 2>&1

# OpenSSL
RUN cd /deps                                                                    1>/log/ssl-1.log 2>&1 && \
    wget https://www.openssl.org/source/openssl-3.2.0.tar.gz                    1>/log/ssl-2.log 2>&1 && \
    tar -xf openssl-3.2.0.tar.gz                                                1>/log/ssl-3.log 2>&1 && \
    cd openssl-3.2.0                                                            1>/log/ssl-4.log 2>&1 && \
    CC=${my_c_compiler} LDFLAGS="-static" \
    perl ./Configure linux-x86_64 --prefix="/deps/ssl" --release no-apps no-autoload-config no-docs no-makedepend no-shared no-tests \
                                                                                1>/log/ssl-5.log 2>&1 && \
    V=1 make -j "$(nproc)"                                                      1>/log/ssl-6.log 2>&1 && \
    V=1 make -j "$(nproc)" install                                              1>/log/ssl-7.log 2>&1 && \
    cp -r /deps/ssl/lib64 /deps/ssl/lib                                         1>/log/ssl-8.log 2>&1

# libcurl
RUN cd /deps                                                                    1>/log/curl-1.log 2>&1 && \
    wget https://curl.se/download/curl-8.5.0.tar.gz                             1>/log/curl-2.log 2>&1 && \
    tar -xf curl-8.5.0.tar.gz                                                   1>/log/curl-3.log 2>&1 && \
    cd curl-8.5.0                                                               1>/log/curl-4.log 2>&1 && \
    CC=${my_c_compiler} LDFLAGS="-static" \
    ./configure --prefix="/deps/curl" --build=x86_64-linux-gnu --disable-debug --enable-optimize --disable-shared --enable-static --disable-libgcc --with-openssl="/deps/ssl" \
                                                                                1>/log/curl-5.log 2>&1 && \
    V=1 make -j "$(nproc)"                                                      1>/log/curl-6.log 2>&1 && \
    V=1 make -j "$(nproc)" install                                              1>/log/curl-7.log 2>&1

# libgit2
RUN cd /deps                                                                    1>/log/git-1.log 2>&1 && \
    wget https://github.com/libgit2/libgit2/archive/refs/tags/v1.7.2.zip        1>/log/git-2.log 2>&1 && \
    unzip v1.7.2.zip                                                            1>/log/git-3.log 2>&1 && \
    mkdir git                                                                   1>/log/git-4.log 2>&1 && \
    cd git                                                                      1>/log/git-5.log 2>&1 && \
    cmake -DCMAKE_C_COMPILER=${my_c_compiler} -DCMAKE_C_FLAGS="-static -isystem /deps/ssl/include" -DCMAKE_VERBOSE_MAKEFILE=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DUSE_HTTPS="OpenSSL" -DOPENSSL_ROOT_DIR=/deps/ssl -DOPENSSL_INCLUDE_DIR=/deps/ssl/include /deps/libgit2-1.7.2 \
                                                                                1>/log/git-6.log 2>&1 && \
    cmake --build . -j "$(nproc)"                                               1>/log/git-7.log 2>&1

# cJSON
RUN cd /deps                                                                    1>/log/cjson-1.log 2>&1 && \
    wget https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.17.tar.gz   1>/log/cjson-2.log 2>&1 && \
    tar -xf v1.7.17.tar.gz                                                      1>/log/cjson-3.log 2>&1
