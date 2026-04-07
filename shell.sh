#!/bin/bash
# Åpner et interaktivt shell i byggmiljøet
# Nyttig for å feilsøke og kjøre enkeltkommandoer

GPL_DIR="/Users/simen/Downloads/GPL"
VOLUME_NAME="dir-x1550-toolchain"
IMAGE_NAME="dir-x1550-builder"

docker run --rm -it \
    -v "$GPL_DIR":/build/GPL \
    --mount "source=${VOLUME_NAME},target=/toolchain,readonly" \
    -w /build/GPL \
    "$IMAGE_NAME" \
    bash
