# syntax=docker/dockerfile:1
# Debian 13 provides a MinGW GCC recent enough for OpenAL Soft's C++20 <format>
# dependency. Debian 12's MinGW GCC 12 does not.
FROM debian:trixie AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        ca-certificates \
        clang \
        cmake \
        file \
        glslang-tools \
        libarchive-dev \
        libfreetype-dev \
        libgl1-mesa-dev \
        libopenal-dev \
        libsdl2-dev \
        libvulkan-dev \
        ninja-build \
        pkg-config \
        qt6-declarative-dev \
        spirv-cross \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ARG EVERSTEAD_TOOLCHAIN_HASH=dev
LABEL everstead.toolchain.hash=$EVERSTEAD_TOOLCHAIN_HASH

WORKDIR /workspace

FROM toolchain AS toolchain-windows

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        autoconf \
        autoconf-archive \
        automake \
        bison \
        curl \
        flex \
        g++-mingw-w64-x86-64 \
        git \
        libltdl-dev \
        libtool \
        libx11-xcb-dev \
        libxcb-cursor-dev \
        libxcb-icccm4-dev \
        libxcb-image0-dev \
        libxcb-xinput-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-render-util0-dev \
        libxcb-shape0-dev \
        libxcb-shm0-dev \
        libxcb-sync-dev \
        libxcb-util-dev \
        libxcb-xfixes0-dev \
        libxcb-xinerama0-dev \
        libxcb-xkb-dev \
        libxkbcommon-x11-dev \
        make \
        python3 \
        python3-venv \
        tar \
        unzip \
        xz-utils \
        zip \
    && rm -rf /var/lib/apt/lists/*

# vcpkg compiles every Windows dependency from source. Left unbounded it can
# exhaust memory on small machines, so it is a build argument rather than a
# hard-coded value.
ARG EVERSTEAD_VCPKG_JOBS=8

ENV VCPKG_ROOT=/opt/vcpkg \
    VCPKG_MAX_CONCURRENCY=$EVERSTEAD_VCPKG_JOBS \
    VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-binary-cache

RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

# Install Windows libraries while building the image. This lets the container
# run as the host user, without writing package-manager state into the source
# checkout.
# The binary and download caches live in BuildKit cache mounts, so changing the
# triplet or the dependency list reuses previously compiled packages instead of
# rebuilding every library from scratch.
COPY game/vcpkg.json /tmp/everstead-vcpkg/vcpkg.json
RUN --mount=type=cache,target=/vcpkg-binary-cache,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    "$VCPKG_ROOT/vcpkg" install \
        --triplet x64-mingw-static \
        --x-manifest-root=/tmp/everstead-vcpkg \
        --x-install-root="$VCPKG_ROOT/installed" \
    || { \
        cat "$VCPKG_ROOT/buildtrees/openal-soft/install-x64-mingw-static-dbg-out.log" 2>/dev/null || true; \
        exit 1; \
    }

# vcpkg creates this lock directory as root during image construction. Remove
# it so post-build dependency copying can recreate it as the host user.
RUN rm -rf /tmp/vcpkg

ARG EVERSTEAD_TOOLCHAIN_HASH=dev
LABEL everstead.toolchain.hash=$EVERSTEAD_TOOLCHAIN_HASH

WORKDIR /workspace

# Cross build for 64-bit ARM Linux using Debian's multiarch arm64 libraries.
FROM toolchain AS toolchain-arm64

RUN dpkg --add-architecture arm64 \
    && apt-get update \
    && apt-get install --yes --no-install-recommends \
        g++-aarch64-linux-gnu \
        libarchive-dev:arm64 \
        libfreetype-dev:arm64 \
        libgl1-mesa-dev:arm64 \
        libopenal-dev:arm64 \
        libsdl2-dev:arm64 \
        libvulkan-dev:arm64 \
        qt6-declarative-dev:arm64 \
        zlib1g-dev:arm64 \
    && rm -rf /var/lib/apt/lists/*

ARG EVERSTEAD_TOOLCHAIN_HASH=dev
LABEL everstead.toolchain.hash=$EVERSTEAD_TOOLCHAIN_HASH

WORKDIR /workspace

FROM toolchain AS toolchain-windows-launcher

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        7zip \
        curl \
        g++-mingw-w64-x86-64 \
        git \
        tar \
        unzip \
        zip \
    && update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix \
    && update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix \
    && rm -rf /var/lib/apt/lists/*

ENV VCPKG_ROOT=/opt/vcpkg \
    VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-binary-cache \
    LANG=C.UTF-8

RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

RUN --mount=type=cache,target=/qt-downloads,sharing=locked \
    if [ ! -f /qt-downloads/qtbase.7z ]; then \
        curl --fail --location --show-error \
            https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_682/qt6_682/qt.qt6.682.win64_mingw/6.8.2-0-202501260838qtbase-Windows-Windows_10_22H2-Mingw-Windows-Windows_10_22H2-X86_64.7z \
            --output /qt-downloads/qtbase.7z; \
    fi \
    && if [ ! -f /qt-downloads/qtdeclarative.7z ]; then \
        curl --fail --location --show-error \
            https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_682/qt6_682/qt.qt6.682.win64_mingw/6.8.2-0-202501260838qtdeclarative-Windows-Windows_10_22H2-Mingw-Windows-Windows_10_22H2-X86_64.7z \
            --output /qt-downloads/qtdeclarative.7z; \
    fi \
    && if [ ! -f /qt-downloads/mingw-runtime.7z ]; then \
        curl --fail --location --show-error \
            https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_682/qt6_682/qt.qt6.682.win64_mingw/6.8.2-0-202501260838MinGW-w64-x86_64-13.1.0-release-posix-seh-msvcrt-rt_v11-rev1-runtime.7z \
            --output /qt-downloads/mingw-runtime.7z; \
    fi \
    && echo "bb8c4df0115c18cfd9f739f771b893475e96b268  /qt-downloads/qtbase.7z" | sha1sum --check \
    && echo "82f47ae5f8035ba41cc46a4f2409eb6251bf9c5c  /qt-downloads/qtdeclarative.7z" | sha1sum --check \
    && echo "54055bfd0c3fe5cc6b4e62e34d9e123ac7a3011e  /qt-downloads/mingw-runtime.7z" | sha1sum --check \
    && 7z x -bd -y -o/opt/qt-windows /qt-downloads/qtbase.7z \
    && 7z x -bd -y -o/opt/qt-windows /qt-downloads/qtdeclarative.7z \
    && 7z x -bd -y -o/opt/qt-windows/bin /qt-downloads/mingw-runtime.7z

COPY launcher/vcpkg.json /tmp/everstead-launcher-vcpkg/vcpkg.json
RUN --mount=type=cache,target=/vcpkg-binary-cache,sharing=locked \
    --mount=type=cache,target=/opt/vcpkg/downloads,sharing=locked \
    "$VCPKG_ROOT/vcpkg" install \
        --triplet x64-mingw-static \
        --x-manifest-root=/tmp/everstead-launcher-vcpkg \
        --x-install-root="$VCPKG_ROOT/launcher-installed"

RUN rm -rf /tmp/vcpkg

ARG EVERSTEAD_TOOLCHAIN_HASH=dev
LABEL everstead.toolchain.hash=$EVERSTEAD_TOOLCHAIN_HASH

WORKDIR /workspace
