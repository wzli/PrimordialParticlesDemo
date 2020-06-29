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

WORKDIR /opt/primordial_particles

# build VSM
COPY VirtualSpaceMeshnet VirtualSpaceMeshnet
RUN echo "add_subdirectory(VirtualSpaceMeshnet)" > CMakeLists.txt \
  && mkdir build \
  && cd build \
  && cmake -Wno-dev .. \
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
COPY --from=0 /opt/primordial_particles/build/sim_node \
  /opt/primordial_particles/build/spiral \
  /usr/local/bin/
# copy required libraries
COPY --from=0 /opt/primordial_particles/build/VirtualSpaceMeshnet/submodules/libzmq/lib/* \
  /usr/lib/x86_64-linux-gnu/libboost_iostreams* \
  /usr/lib/x86_64-linux-gnu/libboost_program_options* \
  /usr/lib/x86_64-linux-gnu/

# default ports
EXPOSE 80/tcp
EXPOSE 11511/udp

# launch using hostname as address
COPY docker/entrypoint.sh .
CMD ./entrypoint.sh
