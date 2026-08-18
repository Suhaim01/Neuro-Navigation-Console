#!/usr/bin/env bash
# Smoke-check navsim: TRAJ on connect, then TRANSFORM stream with image→tracker offset.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PORT="${NNC_NAVSIM_SMOKE_PORT:-18955}"
RATE="${NNC_NAVSIM_SMOKE_RATE:-30}"

if [[ ! -x build/navsim ]]; then
  echo "build/navsim missing — running ./build.sh"
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

./build/navsim --port "${PORT}" --rate "${RATE}" >/tmp/navsim_smoke.log 2>&1 &
NAVSIM_PID=$!
sleep 0.25
if ! kill -0 "${NAVSIM_PID}" 2>/dev/null; then
  echo "navsim failed to start:" >&2
  cat /tmp/navsim_smoke.log >&2
  exit 1
fi

python3 - "${PORT}" <<'PY'
import socket, struct, sys, math

port = int(sys.argv[1])
s = socket.create_connection(("127.0.0.1", port), 2)
s.settimeout(0.6)
data = b""
while len(data) < 208 + 106:
    chunk = s.recv(65536)
    if not chunk:
        break
    data += chunk
s.close()

if len(data) < 208 + 106:
    raise SystemExit(f"short read: {len(data)} bytes")

traj = data[:208]
xform = data[208 : 208 + 106]

def type_field(msg: bytes) -> bytes:
    return msg[2:14].split(b"\0", 1)[0]

if type_field(traj) != b"TRAJ":
    raise SystemExit(f"expected TRAJ, got {type_field(traj)!r}")
if type_field(xform) != b"TRANSFORM":
    raise SystemExit(f"expected TRANSFORM, got {type_field(xform)!r}")

# IGTL TRANSFORM body: 12 big-endian floats, column-major 3x4
body = xform[58:]
floats = [struct.unpack(">f", body[i : i + 4])[0] for i in range(0, 48, 4)]
tx, ty, tz = floats[9], floats[10], floats[11]
sx, sy, sz = floats[6], floats[7], floats[8]

# 2e ground truth at phase 0: tip (40,-25,15), shaft Ry(30°)*+Z
if abs(tx - 40.0) > 1e-2 or abs(ty - (-25.0)) > 1e-2 or abs(tz - 15.0) > 1e-2:
    raise SystemExit(f"unexpected tip {tx},{ty},{tz}")
if abs(sx - 0.5) > 1e-2 or abs(sy) > 1e-2 or abs(sz - math.cos(math.radians(30))) > 1e-2:
    raise SystemExit(f"unexpected shaft {sx},{sy},{sz}")

print("navsim smoke ok")
print(f"  bytes={len(data)} TRAJ=208 TRANSFORM>={106}")
print(f"  first tip=({tx:.1f},{ty:.1f},{tz:.1f}) shaft≈({sx:.3f},{sy:.3f},{sz:.3f})")
print("  hex TRAJ type+name:", traj[2:34].hex())
print("  hex TRANSFORM type+name:", xform[2:34].hex())
PY

echo "navsim log:"
cat /tmp/navsim_smoke.log
