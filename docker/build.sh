#!/bin/sh
tag=${1:-particle_life_sim}
build_context=$(realpath $(dirname ${0})/..)
docker build -f ${build_context}/docker/Dockerfile -t ${tag} ${build_context}

