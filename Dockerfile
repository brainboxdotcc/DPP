FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --no-install-recommends -y libssl-dev=1.1.1f-1ubuntu2.8 zlib1g-dev=1:1.2.11.dfsg-2ubuntu1.2 libsodium-dev=1.0.18-1 libopus-dev=1.3.1-0ubuntu1 cmake=3.16.3-1ubuntu1 pkg-config=0.29.1-0ubuntu4 g++=4:9.3.0-1ubuntu2 gcc=4:9.3.0-1ubuntu2 git=1:2.25.1-1ubuntu3.2 make=4.2.1-1.2 && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/DPP

COPY . .

WORKDIR /usr/src/DPP/build

RUN cmake .. -DDPP_BUILD_TEST=OFF
RUN make -j "$(nproc)"
RUN make install
