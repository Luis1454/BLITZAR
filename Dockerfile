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

RUN cmake --preset cuda-runtime

RUN cmake --build --preset cuda-runtime --parallel "$(nproc)" --target blitzar-headless blitzar-server

FROM docker.io/nvidia/cuda:12.6.0-runtime-ubuntu22.04 AS runtime

WORKDIR /blitzar

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /blitzar/build-cuda/blitzar /blitzar/
COPY --from=builder /blitzar/build-cuda/blitzar-headless /blitzar/
COPY --from=builder /blitzar/build-cuda/blitzar-server /blitzar/
COPY --from=builder /blitzar/simulation.ini /blitzar/

ENV LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH}

ENTRYPOINT ["/blitzar/blitzar-headless"]
CMD ["--config", "/blitzar/simulation.ini"]
