FROM ubuntu:focal@sha256:ed4a42283d9943135ed87d4ee34e542f7f5ad9ecf2f244870e23122f703f91c2

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --no-install-recommends -y libssl-dev zlib1g-dev libsodium-dev libopus-dev cmake pkg-config g++ gcc git make && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/DPP

COPY . .

WORKDIR /usr/src/DPP/build

RUN cmake .. -DDPP_BUILD_TEST=OFF
RUN make -j "$(nproc)"
RUN make install
