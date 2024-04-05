FROM debian:12-slim

# setup docker outside of docker (DooD)
RUN apt-get update && \
    apt-get install -y ca-certificates curl zip && \
    install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc && \
    chmod a+r /etc/apt/keyrings/docker.asc && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] \
          https://download.docker.com/linux/debian bookworm stable" | \
    tee /etc/apt/sources.list.d/docker.list >/dev/null

# env
RUN apt-get update && apt-get install -y docker-ce docker-ce-cli sudo gdb gcc clang make
