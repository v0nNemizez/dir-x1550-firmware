#!/bin/bash
# Bygger MIPS Big Endian uClibc toolchain fra scratch
# Output: Docker named volume "dir-x1550-toolchain"
#         → /toolchain/msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206/
#
# Merk: Toolchain lagres i Docker-volume (Linux ext4) fordi macOS-filsystemet
# er case-insensitivt og crosstool-ng krever case-sensitivt filsystem.
#
# Forventet byggetid: 45–90 minutter
# Forventet størrelse: ~500MB

set -e

VOLUME_NAME="dir-x1550-toolchain"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="dir-x1550-toolchain-builder"
TOOLCHAIN_NAME="msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206"

echo "================================================================"
echo " DIR-X1550 Toolchain Builder"
echo " Mål: Docker volume '$VOLUME_NAME'"
echo "      → /toolchain/$TOOLCHAIN_NAME"
echo "================================================================"
echo ""

# Sjekk om toolchain allerede er bygget
if docker run --rm \
    --mount "source=${VOLUME_NAME},target=/toolchain" \
    alpine test -f "/toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc" 2>/dev/null; then
    echo "Toolchain finnes allerede i volume '$VOLUME_NAME'!"
    docker run --rm \
        --mount "source=${VOLUME_NAME},target=/toolchain" \
        alpine "/toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc" --version
    exit 0
fi

# Opprett volume hvis det ikke finnes
docker volume create "$VOLUME_NAME" > /dev/null

# Bygg Docker-image
echo ">>> Steg 1/3: Bygger Docker-image (crosstool-ng)..."
echo "    (dette tar 2–5 minutter)"
docker build \
    -f "$PROJECT_DIR/Dockerfile.toolchain" \
    -t "$IMAGE_NAME" \
    "$PROJECT_DIR"

echo ""
echo ">>> Steg 2/3: Kompilerer toolchain..."
echo "    GCC 12.3.0 + uClibc-ng 1.0.48 + MIPS32 BE"
echo "    Dette tar 45–90 minutter. Logg følger under:"
echo "----------------------------------------------------------------"

# Kjør toolchain-bygg med Docker volume
docker run --rm \
    --mount "source=${VOLUME_NAME},target=/toolchain" \
    "$IMAGE_NAME"

echo ""
echo ">>> Steg 3/3: Verifiserer toolchain..."

if docker run --rm \
    --mount "source=${VOLUME_NAME},target=/toolchain" \
    alpine test -f "/toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc"; then
    echo ""
    echo "================================================================"
    echo " Toolchain bygget OK!"
    echo "================================================================"
    docker run --rm \
        --mount "source=${VOLUME_NAME},target=/toolchain" \
        alpine "/toolchain/${TOOLCHAIN_NAME}/bin/mips-linux-gcc" --version
    echo ""
    echo "Plassering: Docker volume '$VOLUME_NAME'"
    echo ""
    echo "Neste steg: kjør ./build.sh for å bygge firmware"
else
    echo ""
    echo "FEIL: Toolchain ble ikke funnet etter bygg."
    echo "Sjekk loggen over for feilmeldinger."
    exit 1
fi
