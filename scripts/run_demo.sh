#!/usr/bin/env bash
# Launch navsim (OpenIGTLink sim server) and nnc_console together.
# Console will not consume the stream until Task 3 (IgtlReceiver) lands.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PORT="${NNC_NAVSIM_PORT:-18944}"
RATE="${NNC_NAVSIM_RATE:-60}"

if [[ ! -x build/navsim || ! -x build/nnc_console ]]; then
  echo "binaries missing — running ./build.sh"
  ./build.sh
fi

NAVSIM_PID=""
cleanup() {
  if [[ -n "${NAVSIM_PID}" ]] && kill -0 "${NAVSIM_PID}" 2>/dev/null; then
    echo "stopping navsim (pid ${NAVSIM_PID})"
    kill "${NAVSIM_PID}" 2>/dev/null || true
    wait "${NAVSIM_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

echo "starting navsim on port ${PORT} (rate ${RATE} Hz)"
./build/navsim --port "${PORT}" --rate "${RATE}" &
NAVSIM_PID=$!

# Give the listener a moment to bind before the console starts.
sleep 0.2
if ! kill -0 "${NAVSIM_PID}" 2>/dev/null; then
  echo "navsim failed to start" >&2
  exit 1
fi

echo "starting nnc_console (from repo root for nnc.env / data/)"
./build/nnc_console "$@"
