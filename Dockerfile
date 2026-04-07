# Byggmiljø for D-Link DIR-X1550 firmware
# Basert på Debian 10 (Buster) – samme som D-Link testet med
FROM --platform=linux/amd64 debian:bullseye

# Unngå interaktive spørsmål under apt
ENV DEBIAN_FRONTEND=noninteractive

# Aktiver 32-bit arkitektur (toolchain er 32-bit)
RUN dpkg --add-architecture i386

# Installer avhengigheter
RUN apt-get update && apt-get install -y \
    # 32-bit kompatibilitetsbiblioteker (kreves av toolchain)
    libc6-i386 \
    lib32stdc++6 \
    lib32gcc-s1 \
    zlib1g:i386 \
    # Byggverktøy
    build-essential \
    gcc \
    g++ \
    make \
    bison \
    flex \
    # Kjerneverktøy
    bc \
    libssl-dev \
    libelf-dev \
    # Filsystem-verktøy (for UBIFS/SquashFS-pakking)
    mtd-utils \
    squashfs-tools \
    # Diverse
    bzip2 \
    tar \
    wget \
    curl \
    git \
    python2 \
    python3 \
    ncurses-dev \
    libncurses5-dev \
    unzip \
    rsync \
    file \
    lzma \
    xz-utils \
    cpio \
    u-boot-tools \
    && rm -rf /var/lib/apt/lists/*

# Sett opp arbeidsmappe
WORKDIR /build

# Standard shell
SHELL ["/bin/bash", "-c"]

# Toolchain forventes montert på /toolchain
# GPL-kildekode forventes montert på /build/GPL
# Output havner i /build/GPL/images/

CMD ["/bin/bash"]
