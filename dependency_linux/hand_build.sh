#!/bin/sh
image_name="hand2_linux_image"

cd $(dirname $0)
cd ..
docker run --rm --user $(id -u):$(id -g) -v $(pwd):/hand ${image_name} sh -c "
    cd /hand/code
    ./build.sh
"