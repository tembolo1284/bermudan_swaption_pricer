# Bermudan Swaption Pricer (C++20 + QuantLib + FastAPI)

A robust, modular project for **Bermudan swaption valuation**, built in **C++20** with [QuantLib](https://www.quantlib.org/).  
It supports calibration of **G2++**, **Hull–White (analytic & tree)**, and **Black–Karasinski** short-rate models, and pricing via **tree** and **finite-difference** engines.

We expose functionality through:

- **C++ library** + CLI (`bermudan_main`)
- **Google Tests** for calibration, swaps, and Bermudan pricing
- **Python API** via **pybind11** + **FastAPI**
- **Dockerized builds** for reproducibility
- CI pipelines for **GitHub Actions** and **Jenkins**

---

## Project Layout

bermudan_swaption_pricer/
├── include/ # Public C++ headers
│ ├── YieldCurveBuilder.hpp
│ ├── SwapBuilder.hpp
│ ├── SwaptionCalibrator.hpp
│ ├── BermudanSwaptionPricer.hpp
├── src/ # Library implementations
│ ├── YieldCurveBuilder.cpp
│ ├── SwapBuilder.cpp
│ ├── SwaptionCalibrator.cpp
│ ├── BermudanSwaptionPricer.cpp
├── bindings/ # pybind11 bindings
│ └── bermudan_bindings.cpp
├── python_api/ # FastAPI service
│ ├── app.py
│ ├── pyproject.toml
│ └── README.md
├── test/ # GoogleTest unit tests
│ ├── test_curve.cpp
│ ├── test_swap.cpp
│ ├── test_bermudan.cpp
│ ├── test_calibration.cpp
├── main.cpp # CLI demo entry point
├── CMakeLists.txt # Build system
├── toolchain.sh # Wrapper script for build/run/test (local & docker)
├── Dockerfile # Multi-stage build (CI, API, runtime)
├── .dockerignore
├── .github/workflows/ci.yml # GitHub Actions workflow
├── Jenkinsfile # Jenkins pipeline
└── README.md


## Installation (Ubuntu)

### Install dependencies
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build git curl \
    libquantlib0-dev python3 python3-pip python3-venv
QuantLib: libquantlib0-dev

Ninja: ninja-build (faster builds)

Google Test: pulled automatically via CMake (no system install needed)

Python FastAPI API: pip install fastapi uvicorn pydantic

Building Locally (CMake + Ninja)
```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces:

build/libbermudan_swaption_pricer.a — core library

build/bermudan_main — CLI demo

build/tests — Google Test suite

build/bermudan_native.*.so — Python extension module (for FastAPI)

Running Tests
```
ctest --test-dir build --output-on-failure
# or directly
./build/tests
```

Running the CLI
```
./build/bermudan_main
```
This prints Bermudan swaption NPVs for ATM, OTM, and ITM strikes using different models.

Running the FastAPI Server (Local)

```
# Make C++ Python extension importable
export PYTHONPATH="$PWD/build:$PYTHONPATH"
```

## Install API deps
cd python_api
pip install -r <(echo -e "fastapi\nuvicorn\npydantic")

# Run FastAPI
```
uvicorn app:app --reload --host 0.0.0.0 --port 8000
```
Open interactive docs: http://localhost:8000/docs

Example request:

```
curl -X POST http://localhost:8000/price \
  -H "Content-Type: application/json" \
  -d '{"date":"2025-07-15","flat_rate":0.035,"model":"hw","engine":"tree","strike_multiplier":1.0}'
```

**Docker Usage**
Build and run inside a container:

```
# Build CI image (lib + tests)
docker build -t bermudan-ci .

# Run tests inside container
docker run --rm bermudan-ci

# Build API container
docker build -t bermudan-api .

# Run API container on port 8000
docker run --rm -p 8000:8000 bermudan-api
```

## Toolchain Script
Use toolchain.sh instead of remembering commands:

```
./toolchain.sh build          # local build
./toolchain.sh test           # local test
./toolchain.sh run-main       # run CLI
./toolchain.sh serve-api      # run FastAPI locally

./toolchain.sh build --in=docker      # docker build
./toolchain.sh test --in=docker       # docker test
./toolchain.sh serve-api --in=docker  # docker API
```

## **Features Covered**
Yield curve construction (flat)

Swap builder (payer, ATM/OTM/ITM strikes)

Calibration of G2++, Hull–White, Black–Karasinski

Bermudan swaption pricing:

Tree engines (TreeSwaptionEngine)

Finite-difference engines (FdHullWhiteSwaptionEngine, FdG2SwaptionEngine)

Unit tests:

Discount curve sanity

Swap NPV parity

Bermudan monotonicity (ITM > ATM > OTM)

Calibration vs market vol diagonal (from paper)

## **CI Pipelines**

GitHub Actions (.github/workflows/ci.yml)

Builds via Docker

Runs unit tests

Smoke tests API container

Jenkinsfile

Same flow as GitHub Actions

Publishes JUnit test reports

## **Roadmap**

 Extend FastAPI with calibration endpoints

 Add JSON input/output for flexible pricing requests

 Support more term structure bootstrapping (beyond flat curve)

 Cross-platform GitHub Actions matrix (Linux/macOS/Windows)
