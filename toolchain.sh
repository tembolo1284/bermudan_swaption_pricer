#!/usr/bin/env bash
set -euo pipefail

# Defaults
MODE="local"          # or 'docker'
BUILD_TYPE="Release"  # or 'Debug'
GENERATOR="Ninja"
API_PORT=8000
DOCKER_CI_IMAGE="bermudan-ci"
DOCKER_API_IMAGE="bermudan-api"
DOCKER_RUNNER_IMAGE="bermudan-runner"

usage() {
  cat <<EOF
Usage: $0 <command> [options]

Commands (local and docker):
  build             Configure & build (CMake, Ninja)
  test              Run unit tests (ctest)
  run-main          Run the CLI example (bermudan_main)
  serve-api         Start FastAPI server (requires built python module)

Docker-only helpers:
  docker-build-ci   Build CI image (C++ lib + tests)
  docker-test       Run tests inside the CI image, export junit.xml
  docker-build-api  Build API image (C++ lib + pybind11 + FastAPI)
  docker-serve-api  Run API container on port ${API_PORT}
  docker-run-main   Build minimal runtime image and run CLI binary

Options:
  --in=local|docker       Where to execute the command (default: local)
  --type=Release|Debug    CMAKE_BUILD_TYPE (default: ${BUILD_TYPE})
  --gen=<generator>       CMake generator (default: ${GENERATOR})
  --port=<port>           API port for serve-api / docker-serve-api (default: ${API_PORT})

Examples:
  $0 build --type=Debug
  $0 test --in=docker
  $0 serve-api --port=8080
  $0 docker-build-api && $0 docker-serve-api --port=9000
EOF
}

# Parse args
CMD="${1:-}"
shift || true
for arg in "$@"; do
  case "$arg" in
    --in=*) MODE="${arg#*=}";;
    --type=*) BUILD_TYPE="${arg#*=}";;
    --gen=*) GENERATOR="${arg#*=}";;
    --port=*) API_PORT="${arg#*=}";;
    -h|--help) usage; exit 0;;
    *) ;;
  esac
done

ensure_local_tools() {
  command -v cmake >/dev/null || { echo "cmake not found"; exit 1; }
  command -v ninja >/dev/null || { echo "ninja not found"; exit 1; }
}

case "${CMD}" in
  build)
    if [[ "${MODE}" == "local" ]]; then
      ensure_local_tools
      cmake -B build -G "${GENERATOR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
      cmake --build build
    elif [[ "${MODE}" == "docker" ]]; then
      docker build -t "${DOCKER_CI_IMAGE}" .
    else
      usage; exit 1
    fi
    ;;

  test)
    if [[ "${MODE}" == "local" ]]; then
      ctest --test-dir build --output-on-failure
    elif [[ "${MODE}" == "docker" ]]; then
      cid=$(docker create "${DOCKER_CI_IMAGE}")
      docker start -a "$cid" || true
      docker cp "$cid:/app/junit.xml" junit.xml || true
      docker rm -v "$cid"
      echo "JUnit (if any) saved to ./junit.xml"
    else
      usage; exit 1
    fi
    ;;

  run-main)
    if [[ "${MODE}" == "local" ]]; then
      ./build/bermudan_main
    elif [[ "${MODE}" == "docker" ]]; then
      # Build a small runtime image and run
      docker build -t "${DOCKER_RUNNER_IMAGE}" --target runtime .
      docker run --rm "${DOCKER_RUNNER_IMAGE}"
    else
      usage; exit 1
    fi
    ;;

  serve-api)
    if [[ "${MODE}" == "local" ]]; then
      # Ensure Python module is importable
      export PYTHONPATH="$PWD/build:${PYTHONPATH:-}"
      (cd python_api && uvicorn app:app --host 0.0.0.0 --port "${API_PORT}")
    elif [[ "${MODE}" == "docker" ]]; then
      docker build -t "${DOCKER_API_IMAGE}" .
      docker run --rm -p "${API_PORT}:8000" "${DOCKER_API_IMAGE}"
    else
      usage; exit 1
    fi
    ;;

  docker-build-ci)
    docker build -t "${DOCKER_CI_IMAGE}" .
    ;;

  docker-test)
    "$0" test --in=docker
    ;;

  docker-build-api)
    docker build -t "${DOCKER_API_IMAGE}" .
    ;;

  docker-serve-api)
    docker run --rm -p "${API_PORT}:8000" "${DOCKER_API_IMAGE}"
    ;;

  docker-run-main)
    docker build -t "${DOCKER_RUNNER_IMAGE}" --target runtime .
    docker run --rm "${DOCKER_RUNNER_IMAGE}"
    ;;

  ""|-h|--help)
    usage
    ;;

  *)
    echo "Unknown command: ${CMD}"
    usage
    exit 1
    ;;
esac

