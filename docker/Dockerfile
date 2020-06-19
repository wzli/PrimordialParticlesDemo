FROM ubuntu:18.04
RUN apt update && apt install -y \
  g++ \
  cmake \
  libboost-all-dev \
  && rm -rf /var/cache/*

WORKDIR /opt/particle_life_sim
COPY . .
RUN mkdir build && cd build && cmake .. && make -j

FROM ubuntu:18.04
COPY --from=0 /opt/particle_life_sim/build/sim_node /usr/local/bin/
COPY --from=0 /opt/particle_life_sim/build/VirtualSpaceMeshnet/submodules/libzmq/lib/* \
  /usr/lib/x86_64-linux-gnu/libboost_iostreams* /usr/lib/x86_64-linux-gnu/

EXPOSE 11511/udp
ENTRYPOINT ["sim_node"]
