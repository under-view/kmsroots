#!/bin/bash

[[ -z "$1" ]] && {
  echo "[x] Please include absolute path to underview libs build_output directory"
  exit 1
}

UNDERVIEW_BUILD_OUTPUT_DIR="$1"
export PKG_CONFIG_PATH="${UNDERVIEW_BUILD_OUTPUT_DIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export LIBGL_DRIVERS_PATH="${UNDERVIEW_BUILD_OUTPUT_DIR}/lib/dri"
export PATH="${UNDERVIEW_BUILD_OUTPUT_DIR}/bin:${PATH}"
export LD_LIBRARY_PATH="${UNDERVIEW_BUILD_OUTPUT_DIR}/lib:${LD_LIBRARY_PATH}"
