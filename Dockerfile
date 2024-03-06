FROM ubuntu:focal@sha256:80ef4a44043dec4490506e6cc4289eeda2d106a70148b74b5ae91ee670e9c35d

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --no-install-recommends -y libssl-dev zlib1g-dev libsodium-dev libopus-dev cmake pkg-config g++ gcc git make && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/DPP

COPY . .

WORKDIR /usr/src/DPP/build

RUN cmake .. -DDPP_BUILD_TEST=OFF
RUN make -j "$(nproc)"
RUN make install
