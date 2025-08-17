#!/usr/bin/env bash
set -euo pipefail

# -------------------------------
# Defaults & helpers
# -------------------------------
IN="${IN:-local}"  # local | docker
ACTION="${1:-help}"
shift || true

# Parse flags like --in=docker
for arg in "$@"; do
  case "$arg" in
    --in=*) IN="${arg#*=}";;
  esac
done

IMAGE_BUILD="bermudan_pricer:build"
IMAGE_RUNTIME="bermudan_pricer:runtime"

require_docker() {
  if ! command -v docker >/dev/null 2>&1; then
    echo "Docker not found. Install it (see README) or run with --in=local" >&2
    exit 127
  fi
}

usage() {
  cat <<EOF
Usage: $0 <command> [--in=local|docker]

Commands:
  build       Configure & build (local) or docker build images
  test        Run unit tests (emits junit.xml)
  run         Run the CLI demo (prints ATM/OTM/ITM)
  serve-api   (reserved) start API container if added later
  clean       Remove local build/ or prune docker images

Examples:
  $0 build
  $0 build --in=docker
  $0 test
  $0 test --in=docker
  $0 run --in=docker
EOF
}

# -------------------------------
# Local actions
# -------------------------------
local_build() {
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build
}

local_test() {
  if [[ -x build/tests ]]; then
    ./build/tests --gtest_output=xml:./junit.xml
  else
    echo "No tests binary found at build/tests; did you build? Falling back to ctest."
    ctest --test-dir build --output-on-failure
    # ctest doesn't emit junit by default; consider CTest JUnit if needed.
  fi
  echo "JUnit (if any) saved to ./junit.xml"
}

local_run() {
  if [[ -x build/bermudan_main ]]; then
    ./build/bermudan_main
  else
    echo "Executable build/bermudan_main not found. Run: $0 build"
    exit 1
  fi
}

# -------------------------------
# Docker actions
# -------------------------------
docker_build() {
  require_docker
  # Build both stages explicitly
  docker build --target build   -t "${IMAGE_BUILD}"   .
  docker build --target runtime -t "${IMAGE_RUNTIME}" .
}

docker_test() {
  require_docker

  # Ensure build image exists (quick no-op if already built)
  docker build --target build -t "${IMAGE_BUILD}" .

  # Run tests inside a disposable container and export JUnit
  CID="$(docker create "${IMAGE_BUILD}" /app/build/tests --gtest_output=xml:/app/junit.xml || true)"
  if [[ -z "$CID" ]]; then
    echo "Failed to create container for tests. Did the build produce /app/build/tests?" >&2
    exit 1
  fi
  # Start the test process (will exit when tests finish)
  docker start -a "$CID" || true

  # Try to copy JUnit out (may not exist if tests didn't run)
  if docker cp "$CID:/app/junit.xml" ./junit.xml 2>/dev/null; then
    echo "JUnit saved to ./junit.xml"
  else
    echo "No JUnit produced at /app/junit.xml (tests may have failed earlier)."
  fi

  docker rm "$CID" >/dev/null || true
}

docker_run() {
  require_docker
  # Ensure runtime image exists
  docker build --target runtime -t "${IMAGE_RUNTIME}" .
  docker run --rm "${IMAGE_RUNTIME}"
}

# -------------------------------
# Dispatch
# -------------------------------
case "$ACTION" in
  build)
    if [[ "$IN" == "docker" ]]; then docker_build; else local_build; fi
    ;;
  test)
    if [[ "$IN" == "docker" ]]; then docker_test; else local_test; fi
    ;;
  run)
    if [[ "$IN" == "docker" ]]; then docker_run; else local_run; fi
    ;;
  serve-api)
    echo "API container not wired yet in this script. (Your FastAPI app can be added later.)"
    ;;
  clean)
    if [[ "$IN" == "docker" ]]; then
      require_docker
      docker image rm -f "${IMAGE_BUILD}" "${IMAGE_RUNTIME}" 2>/dev/null || true
    else
      rm -rf build
    fi
    ;;
  help|--help|-h)
    usage
    ;;
  *)
    echo "Unknown command: $ACTION"; usage; exit 2;;
esac

