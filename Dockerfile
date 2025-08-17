# -------- Base images --------
FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

# System deps for compiling C++ + pybind11 ext + finding QuantLib via pkg-config
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates \
    python3 python3-pip python3-dev \
    pkg-config \
    libquantlib0-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source
COPY . /app

# Configure & build
# We set PYTHONPATH for the pybind11 module import path at runtime in the final image,
# but define it here too so tools that try to expand it don't warn.
ENV PYTHONPATH=/app/build

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build

# Optionally run tests during build (uncomment if you want CI to fail on tests)
# RUN ctest --test-dir build --output-on-failure

# -------- Runtime image --------
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Only what we need to run binaries + Python module
RUN apt-get update && apt-get install -y --no-install-recommends \
    libquantlib0v5 \
    python3 \
 && rm -rf /var/lib/apt/lists/*

# Put binaries on PATH
COPY --from=build /app/build/bermudan_main /usr/local/bin/bermudan_main

# Install the native Python extension & set import path
# (We keep the .so in a dedicated dir and add it to PYTHONPATH)
RUN mkdir -p /opt/bermudan
COPY --from=build /app/build/bermudan_native*.so /opt/bermudan/
ENV PYTHONPATH=/opt/bermudan

# If you want to run the CLI by default:
CMD ["bermudan_main"]
# For serving the FastAPI (if you later copy the app), you'd switch CMD to uvicorn.

