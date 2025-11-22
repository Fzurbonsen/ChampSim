# Dockerfile
FROM ubuntu:22.04

# Install build tools and libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    libfmt-dev \
    nlohmann-json3-dev \
    git \
    wget \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    zlib1g-dev \
    libbz2-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install --no-cache-dir numpy pandas

WORKDIR /ChampSim

# Copy sources
COPY ./inc/ /ChampSim/inc/
COPY ./src/ /ChampSim/src/
COPY ./vcpkg/ /ChampSim/vcpkg/
COPY ./branch/ /ChampSim/branch/
COPY ./btb/ /ChampSim/btb/
COPY ./config/ /ChampSim/config/
COPY ./docs/ /ChampSim/docs/
COPY ./dpc4/ /ChampSim/dpc4/
COPY ./prefetcher/  /ChampSim/prefetcher/
COPY ./replacement/ /ChampSim/replacement/
COPY ./test/ /ChampSim/test/
COPY ./tracer/ /ChampSim/tracer/
COPY ./.clang-format /ChampSim/.clang-format
COPY ./.clang-tidy /ChampSim/.clang-tidy
COPY ./Makefile /ChampSim/Makefile
COPY ./champsim_config.json /ChampSim/champsim_config.json
COPY ./config.sh /ChampSim/config.sh
COPY ./global.options /ChampSim/global.options
COPY ./module.options /ChampSim/module.options
COPY ./vcpkg.json /ChampSim/vcpkg.json

# Install vcpkg
RUN vcpkg/bootstrap-vcpkg.sh
RUN vcpkg/vcpkg install
