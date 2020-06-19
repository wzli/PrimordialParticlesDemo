FROM ubuntu:18.04
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
COPY . .
RUN mkdir build && cd build && cmake .. && make -j

FROM ubuntu:18.04
COPY --from=0 /opt/particle_life_sim/build/sim_node /usr/local/bin/
COPY --from=0 /opt/particle_life_sim/build/VirtualSpaceMeshnet/submodules/libzmq/lib/* \
  /usr/lib/x86_64-linux-gnu/libboost_iostreams* /usr/lib/x86_64-linux-gnu/

EXPOSE 80/tcp
EXPOSE 11511/udp

CMD sim_node -p 80 -b 11511 -n $(hostname) $(hostname -I)

