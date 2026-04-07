#!/bin/bash
# Bygger DIR-X1550 firmware lokalt
# Toolchain hentes fra lokalt Docker volume, eller fra ghcr.io hvis volume mangler.

set -e

GPL_DIR="/Users/simen/Downloads/GPL"
VOLUME_NAME="dir-x1550-toolchain"
TOOLCHAIN_NAME="msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="dir-x1550-builder"
GHCR_IMAGE="ghcr.io/v0nnemizez/dir-x1550-toolchain"

# --- Sjekker ---

if [ ! -d "$GPL_DIR" ]; then
    echo "FEIL: GPL-mappen finnes ikke: $GPL_DIR"
    exit 1
fi

# --- Sørg for at toolchain-volumet finnes ---

TOOLCHAIN_BIN="/toolchain/${TOOLCHAIN_NAME}/bin/msdk-linux-gcc"

if docker run --rm \
    --mount type=volume,source=${VOLUME_NAME},target=/toolchain \
    dir-x1550-toolchain-builder \
    test -f "$TOOLCHAIN_BIN" 2>/dev/null; then
    echo ">>> Toolchain funnet i lokalt volume."
else
    echo ">>> Lokalt toolchain-volume mangler. Henter fra ghcr.io..."

    # Finn riktig hash basert på gjeldende konfig
    HASH=$(sha256sum \
        "$PROJECT_DIR/ct-ng.defconfig" \
        "$PROJECT_DIR/uclibc.config" \
        "$PROJECT_DIR/Dockerfile.toolchain" | sha256sum | cut -c1-12)

    IMAGE_REF="${GHCR_IMAGE}:${HASH}"
    echo "    Image: $IMAGE_REF"

    docker pull "$IMAGE_REF" || {
        echo ""
        echo "FEIL: Fant ikke toolchain-image på ghcr.io."
        echo "Alternativ 1: Kjør ./build-toolchain.sh for å bygge lokalt (~60 min)"
        echo "Alternativ 2: Kjør 'Build Toolchain' på GitHub Actions, vent til ferdig"
        exit 1
    }

    # Pakk ut toolchain fra imaget til volumet
    docker volume create "$VOLUME_NAME" > /dev/null
    docker create --name tc-extract "$IMAGE_REF" 2>/dev/null || true
    docker cp tc-extract:/toolchain/. /tmp/tc-extract-tmp/
    # Kopier inn i volumet via builder-imaget
    docker run --rm \
        -v /tmp/tc-extract-tmp:/src \
        --mount type=volume,source=${VOLUME_NAME},target=/toolchain \
        dir-x1550-toolchain-builder \
        cp -a /src/. /toolchain/
    docker rm tc-extract
    rm -rf /tmp/tc-extract-tmp
    echo ">>> Toolchain hentet og klar."
fi

# --- Bygg Docker-image om det ikke finnes ---

if ! docker image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    echo ">>> Bygger firmware-builder image..."
    docker build -t "$IMAGE_NAME" "$PROJECT_DIR"
fi

# --- Anvend patches på GPL-kildekode ---

for patch in "$PROJECT_DIR/patches/"*.patch; do
    [ -f "$patch" ] || continue
    patch_name=$(basename "$patch")
    if patch --dry-run -p1 -R -s --dir "$GPL_DIR" < "$patch" > /dev/null 2>&1; then
        echo "    Patch allerede anvendt: $patch_name"
    else
        echo "    Anvender patch: $patch_name"
        patch -p1 --dir "$GPL_DIR" < "$patch"
    fi
done

# --- Kjør bygget ---

echo ""
echo ">>> Starter firmware-bygg..."
echo "    GPL:      $GPL_DIR"
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
        /toolchain/${TOOLCHAIN_NAME}/bin/msdk-linux-gcc --version

        echo '--- Starter bygg ---'
        make HOSTCFLAGS=-fcommon KCFLAGS='-Wno-array-bounds -Wno-stringop-overflow -Wno-stringop-overread -Wno-missing-attributes -Wno-return-local-addr -Wno-incompatible-pointer-types -Wno-unused-function' 2>&1 | tee /build/GPL/build.log

        echo '--- Ferdig ---'
        ls -lh /build/GPL/images/ 2>/dev/null || echo 'Ingen images/ mappe funnet'
    "

echo ""
echo ">>> Bygg fullført. Firmware: $GPL_DIR/images/"
