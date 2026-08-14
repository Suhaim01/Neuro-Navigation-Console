# Build and test

## Prerequisites

- CMake ≥ 3.16
- C++17 compiler (GCC or Clang)
- Qt 5 (`Widgets`, `OpenGL`, `Test`) — e.g. Ubuntu: `qtbase5-dev` `libqt5opengl5-dev`

Optional local volume (gitignored): place `MRHead.nii` under `data/`.

## Configure and build

```bash
./build.sh
```

Equivalent:

```bash
cmake --preset default
cmake --build --preset default
```

Debug preset: `cmake --preset debug` then `cmake --build --preset debug`.

Binary: `build/nnc_console` (or `build-debug/nnc_console`).

## Run the console

```bash
./build/nnc_console
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Committed fixtures under `tests/fixtures/` are enough for CI. To also exercise `data/MRHead.nii` locally:

```bash
NNC_DATA=./data ctest --test-dir build --output-on-failure
```

Optional: `NNC_FIXTURES` overrides the fixtures directory (defaults to `tests/fixtures` via CMake).

## Continuous integration

GitHub Actions builds on `ubuntu-latest` (x86_64) and `ubuntu-24.04-arm` (aarch64). See `.github/workflows/ci.yml`.
