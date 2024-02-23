#!/bin/sh
image_name="hand2_linux_image"

cd $(dirname $0)
cd ..
docker run -it --rm --security-opt seccomp=unconfined -v $(pwd):/hand ${image_name} sh -c "
    cd /hand/build
    gdb --args hand_gcc $*
"