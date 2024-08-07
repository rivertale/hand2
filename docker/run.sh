#!/bin/sh
cd "$(dirname "$0")"
base_dir="$(pwd "$0")"
image_name="hand2_linux"

# build docker if needed
cd "${base_dir}"
docker inspect --type=image "${image_name}" 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]
then
    echo "Unable to find docker image '${image_name}', try to build it"
    docker build -t "${image_name}" .
    if [ $? -eq 0 ]; then echo "Output docker image: ${image_name}"; fi
fi

# parse arguments
args=""
for arg in "$@"
do
    args="${args} \"${arg}\""
done

# set working directory
cd "${base_dir}"
if [ -d ../build ];
then
    cd ../build
else
    cd ..
fi

# run hand2
docker run --rm \
    --volume "$(pwd):$(pwd)" \
    --volume "/var/run/docker.sock:/var/run/docker.sock" \
    --workdir "$(pwd)" \
    "${image_name}" sh -c "
        groupadd --gid $(id -g) ghand
        useradd --uid $(id -u) --gid $(id -g) uhand
        chmod 666 /var/run/docker.sock
        exec sudo --user=uhand ./hand ${args}
    "
