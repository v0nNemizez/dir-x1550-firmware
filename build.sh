#!/bin/bash
# Bygger DIR-X1550 firmware i Docker
# Kjøres fra Mac-terminalen

set -e

GPL_DIR="/Users/simen/Downloads/GPL"
VOLUME_NAME="dir-x1550-toolchain"
TOOLCHAIN_NAME="msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="dir-x1550-builder"

# --- Sjekker ---

if [ ! -d "$GPL_DIR" ]; then
    echo "FEIL: GPL-mappen finnes ikke: $GPL_DIR"
    exit 1
fi

if ! docker volume inspect "$VOLUME_NAME" > /dev/null 2>&1; then
    echo "FEIL: Docker volume '$VOLUME_NAME' ikke funnet."
    echo "Kjør ./build-toolchain.sh først."
    exit 1
fi

if ! docker run --rm \
    --mount type=volume,source=${VOLUME_NAME},target=/toolchain \
    alpine test -f "/toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc" 2>/dev/null; then
    echo "FEIL: Toolchain ikke ferdig bygget i volume '$VOLUME_NAME'."
    echo "Kjør ./build-toolchain.sh først."
    exit 1
fi

# --- Bygg Docker-image om det ikke finnes ---

if ! docker image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    echo ">>> Bygger Docker-image..."
    docker build -t "$IMAGE_NAME" "$PROJECT_DIR"
fi

# --- Kjør bygget ---

echo ">>> Starter firmware-bygg..."
echo "    GPL:       $GPL_DIR"
echo "    Toolchain: volume:$VOLUME_NAME"
echo "    Output:    $GPL_DIR/images/"
echo ""

docker run --rm \
    -v "$GPL_DIR":/build/GPL \
    --mount type=volume,source=${VOLUME_NAME},target=/toolchain,readonly \
    -w /build/GPL \
    "$IMAGE_NAME" \
    bash -c "
        echo '--- Verifiserer toolchain ---'
        /toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc --version

        echo '--- Starter bygg ---'
        make 2>&1 | tee /build/GPL/build.log

        echo '--- Ferdig ---'
        ls -lh /build/GPL/images/ 2>/dev/null || echo 'Ingen images/ mappe funnet'
    "

echo ""
echo ">>> Bygg fullført. Firmware: $GPL_DIR/images/"
