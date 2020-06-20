FROM ubuntu:18.04

# fetch dependencies
RUN apt update
RUN apt install -y \
  vim \
  wget \
  net-tools \
  iputils-ping \
  g++ \
  cmake \
  libboost-all-dev

WORKDIR /opt/particle_life_sim

# build VSM
COPY VirtualSpaceMeshnet VirtualSpaceMeshnet
RUN echo "add_subdirectory(VirtualSpaceMeshnet)" > CMakeLists.txt \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make -j

# build spiral function
COPY docker/spiral.c .
RUN gcc spiral.c -lm -o build/spiral

# build simulation mesh node
COPY CMakeLists.txt .
COPY include include
COPY src src
RUN cd build && cmake .. && make -j

# create output container
FROM ubuntu:18.04
# copy built binaries
COPY --from=0 /opt/particle_life_sim/build/sim_node \
  /opt/particle_life_sim/build/spiral \
  /usr/local/bin/
# copy required libraries
COPY --from=0 /opt/particle_life_sim/build/VirtualSpaceMeshnet/submodules/libzmq/lib/* \
  /usr/lib/x86_64-linux-gnu/libboost_iostreams* \
  /usr/lib/x86_64-linux-gnu/libboost_program_options* \
  /usr/lib/x86_64-linux-gnu/

# default ports
EXPOSE 80/tcp
EXPOSE 11511/udp

# launch using hostname as address
CMD address=$(hostname -I); sim_node --http-port 80 --mesh-port 11511 $(spiral ${address##*.} 30) \
  --name $(hostname) --bootstrap-peer ${address%.*}.2:11511 $address
