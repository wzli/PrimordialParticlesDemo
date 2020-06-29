ADDRESS=${ADDRESS:-$(hostname -i)}
EXTERNAL_ADDRESS=${EXTERNAL_ADDRESS:-$(hostname)}

if [ -n "${GOOGLE_CLOUD_PLATFORM+x}" ]; then
    EXTERNAL_ADDRESS=$(curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip)
fi

HTTP_PORT=${HTTP_PORT:-80}
MESH_PORT=${MESH_PORT:-11511}
BOOTSTRAP_PEER=${BOOTSTRAP_PEER:-${ADDRESS%.*}.2:$MESH_PORT}

SIM_RADIUS=${SIM_RADIUS:-20}
SIM_DENSITY=${SIM_DENSITY:-0.04}
SIM_INTERVAL=${SIM_INTERVAL:-50}
CONNECTION_DEGREE=${CONNECTION_DEGREE:-4}
DISTANCE_GAIN=${DISTANCE_GAIN:-0.002}
COORDS=${COORDS:-$(spiral $((${ADDRESS##*.} - 2)) $SIM_RADIUS)}

sim_node --http-port $HTTP_PORT --mesh-port $MESH_PORT --address $ADDRESS \
  --name $EXTERNAL_ADDRESS --bootstrap-peer $BOOTSTRAP_PEER $COORDS \
  --sim-radius $SIM_RADIUS --sim-density $SIM_DENSITY --sim-interval $SIM_INTERVAL \
  --connection-degree $CONNECTION_DEGREE --distance-gain $DISTANCE_GAIN \
  $NAME_AS_LINK || sleep infinity
