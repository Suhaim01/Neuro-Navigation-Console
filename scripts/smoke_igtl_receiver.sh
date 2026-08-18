#!/usr/bin/env bash
# Smoke-check IgtlReceiver: navsim + nnc_console --igtl-smoke (TRAJ + TRANSFORM).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PORT="${NNC_NAVSIM_SMOKE_PORT:-18956}"
RATE="${NNC_NAVSIM_SMOKE_RATE:-30}"

if [[ ! -x build/navsim || ! -x build/nnc_console ]]; then
  echo "binaries missing — running ./build.sh"
  ./build.sh
fi

NAVSIM_PID=""
cleanup() {
  if [[ -n "${NAVSIM_PID}" ]] && kill -0 "${NAVSIM_PID}" 2>/dev/null; then
    kill "${NAVSIM_PID}" 2>/dev/null || true
    wait "${NAVSIM_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

./build/navsim --port "${PORT}" --rate "${RATE}" >/tmp/navsim_igtl_smoke.log 2>&1 &
NAVSIM_PID=$!
sleep 0.25
if ! kill -0 "${NAVSIM_PID}" 2>/dev/null; then
  echo "navsim failed to start:" >&2
  cat /tmp/navsim_igtl_smoke.log >&2
  exit 1
fi

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
./build/nnc_console --igtl-smoke --igtl-port "${PORT}"

echo "navsim log:"
cat /tmp/navsim_igtl_smoke.log
