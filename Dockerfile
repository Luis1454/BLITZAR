# @file Dockerfile
# @brief BLITZAR GPU N-Body Simulation Engine
# @description Reproducible CUDA build and runtime image

FROM docker.io/nvidia/cuda:12.6.0-devel-ubuntu22.04 AS builder

ARG CMAKE_VERSION=3.27.6
ARG RUST_TOOLCHAIN=1.94.0

WORKDIR /blitzar

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    git \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO /tmp/cmake.sh https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && chmod +x /tmp/cmake.sh \
    && /tmp/cmake.sh --skip-license --prefix=/usr/local \
    && rm -f /tmp/cmake.sh

RUN wget -qO- https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain ${RUST_TOOLCHAIN}
ENV PATH=/root/.cargo/bin:$PATH

COPY . .

ENV CMAKE_BUILD_TYPE=Release
ENV BLITZAR_PROFILE=dev
ENV BLITZAR_BUILD_TESTS=OFF
ENV BLITZAR_BUILD_SERVER_DAEMON=ON
ENV BLITZAR_BUILD_HEADLESS_BINARY=ON
ENV BLITZAR_BUILD_CLIENT_HOST=OFF
ENV BLITZAR_BUILD_WEB_GATEWAY=OFF
ENV BLITZAR_BUILD_CLIENT_MODULES=OFF

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -DBLITZAR_PROFILE=${BLITZAR_PROFILE} \
    -DBLITZAR_BUILD_TESTS=${BLITZAR_BUILD_TESTS} \
    -DBLITZAR_BUILD_SERVER_DAEMON=${BLITZAR_BUILD_SERVER_DAEMON} \
    -DBLITZAR_BUILD_HEADLESS_BINARY=${BLITZAR_BUILD_HEADLESS_BINARY} \
    -DBLITZAR_BUILD_CLIENT_HOST=${BLITZAR_BUILD_CLIENT_HOST} \
    -DBLITZAR_BUILD_WEB_GATEWAY=${BLITZAR_BUILD_WEB_GATEWAY} \
    -DBLITZAR_BUILD_CLIENT_MODULES=${BLITZAR_BUILD_CLIENT_MODULES}

RUN cmake --build build --parallel "$(nproc)" --target blitzar blitzar-headless blitzar-server

FROM docker.io/nvidia/cuda:12.6.0-runtime-ubuntu22.04 AS runtime

WORKDIR /blitzar

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /blitzar/build/blitzar /blitzar/
COPY --from=builder /blitzar/build/blitzar-headless /blitzar/
COPY --from=builder /blitzar/build/blitzar-server /blitzar/
COPY --from=builder /blitzar/simulation.ini /blitzar/

ENV LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH}

ENTRYPOINT ["/blitzar/blitzar-headless"]
CMD ["--config", "/blitzar/simulation.ini"]
