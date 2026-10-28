#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build the Mesa host toolchain Adrenoline uses for shader work.
#
# Nothing is written outside $ROOT. Do not use /tmp: on this machine it is
# tmpfs and any build tree there is lost on reboot.
#
#   ADRENOLINE_WORK=/data/opencode ./tools/build-mesa-host.sh
#
# Produces, under $ROOT/mesa/mesa-$MESA_VER/build:
#   src/compiler/glsl/glsl_compiler                    GLSL -> NIR
#   src/freedreno/computerator/computerator            IR3 assembly, no a5xx
#   src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so   fake msm device
#
set -euo pipefail

ROOT="${ADRENOLINE_WORK:-/data/opencode}"
MESA_VER="${MESA_VER:-26.2.6}"
SRC="${ROOT}/mesa/mesa-${MESA_VER}"
TARBALL="${ROOT}/mesa/mesa-${MESA_VER}.tar.xz"
URL="https://archive.mesa3d.org/mesa-${MESA_VER}.tar.xz"
VENV="${ROOT}/venv"
JOBS="${JOBS:-$(nproc)}"

mkdir -p "${ROOT}/mesa"

if [ ! -d "${SRC}" ]; then
    if [ ! -f "${TARBALL}" ]; then
        echo "fetching ${URL}"
        curl -sS --max-time 600 -o "${TARBALL}" "${URL}"
    fi
    tar -xf "${TARBALL}" -C "${ROOT}/mesa"
fi

if [ ! -x "${VENV}/bin/python" ]; then
    python3 -m venv "${VENV}"
    "${VENV}/bin/pip" -q install --upgrade pip
fi
"${VENV}/bin/pip" -q install mako pyyaml packaging

export PATH="${VENV}/bin:${PATH}"

# Desktop GLSL, not GLES: the AIR/ MSL lowering targets GLSL 430, and the
# GLES frontend is not needed for that path. Leaving EGL and GLX off keeps
# the build to the compiler and the freedreno driver only.
meson setup "${SRC}/build" "${SRC}" \
    --wipe \
    -Dbuildtype=release -Dwerror=false \
    -Dgallium-drivers=freedreno -Dvulkan-drivers= \
    -Dglx=disabled -Degl=disabled -Dgles1=disabled -Dgles2=disabled \
    -Dtools=glsl,nir,freedreno,drm-shim \
    -Dllvm=disabled -Dzstd=disabled

ninja -C "${SRC}/build" -j"${JOBS}" \
    src/compiler/glsl/glsl_compiler \
    src/freedreno/computerator/computerator \
    src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so

echo
echo "glsl_compiler:  ${SRC}/build/src/compiler/glsl/glsl_compiler"
echo "computerator:   ${SRC}/build/src/freedreno/computerator/computerator"
echo "noop shim:      ${SRC}/build/src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so"
