#!/bin/sh
image_name="hand2_linux"

# build docker
cd $(dirname "$0")
docker inspect --type=image ${image_name} 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]
then
    echo "Unable to find docker image '${image_name}', try to build it"
    docker build -t ${image_name} .
    if [ $? -eq 0 ]; then echo "Output docker image: ${image_name}"; fi
fi

# parse arguments
args=""
for arg in "$@"
do
    args="${args} \"${arg}\""
done

# debug hand2
cd $(dirname "$0")
cd ..
docker run -it --rm --security-opt seccomp=unconfined --user $(id -u):$(id -g) --volume $(pwd):/hand ${image_name} sh -c "
    cd /hand/build
    gdb --args hand_gcc ${args}
"