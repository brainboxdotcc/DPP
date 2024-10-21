FROM ubuntu:noble@sha256:99c35190e22d294cdace2783ac55effc69d32896daaa265f0bbedbcde4fbe3e5

RUN apt-get update && apt-get install --no-install-recommends -y libssl-dev zlib1g-dev libopus-dev cmake pkg-config g++ gcc git make && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/DPP

COPY . .

WORKDIR /usr/src/DPP/build

RUN cmake .. -DDPP_BUILD_TEST=OFF
RUN make -j "$(nproc)"
RUN make install
