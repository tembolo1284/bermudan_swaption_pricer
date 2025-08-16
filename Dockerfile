# syntax=docker/dockerfile:1

#####################
# Build stage
#####################
FROM ubuntu:22.04 AS build

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates \
    python3 python3-pip python3-venv \
    libquantlib0-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

# Build C++ library and the pybind11 extension
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build

# Create venv and install API deps
RUN python3 -m venv /venv
ENV PATH=/venv/bin:$PATH
WORKDIR /app/python_api
RUN pip install --no-cache-dir -U pip
RUN pip install --no-cache-dir fastapi uvicorn pydantic

#####################
# Runtime stage
#####################
FROM ubuntu:22.04 AS runtime
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libquantlib0v5 python3 \
 && rm -rf /var/lib/apt/lists/*

# copy venv and app
COPY --from=build /venv /venv
ENV PATH=/venv/bin:$PATH

WORKDIR /app
COPY python_api /app/python_api
COPY --from=build /app/build/bermudan_native*.so /app/build/

# Make the module importable
ENV PYTHONPATH=/app/build:$PYTHONPATH

EXPOSE 8000
CMD ["uvicorn", "python_api.app:app", "--host", "0.0.0.0", "--port", "8000"]

