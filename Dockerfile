FROM ubuntu:focal@sha256:f5c3e53367f142fab0b49908550bdcdc4fb619d2f61ec1dfa60d26e0d59ac9e7

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --no-install-recommends -y libssl-dev zlib1g-dev libsodium-dev libopus-dev cmake pkg-config g++ gcc git make && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/DPP

COPY . .

WORKDIR /usr/src/DPP/build

RUN cmake .. -DDPP_BUILD_TEST=OFF
RUN make -j "$(nproc)"
RUN make install
