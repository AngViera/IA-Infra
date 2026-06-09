# ── Stage 1: Build ────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    libgomp1 \
    git \
    curl \
    ca-certificates \
    make \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY . .

RUN cmake -B cmake-build -DCMAKE_BUILD_TYPE=Release . && \
    cmake --build cmake-build --parallel $(nproc)

# ── Stage 2: Runtime ──────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libgomp1 \
    libssl3 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /build/cmake-build/mancala_server /app/mancala_server
COPY --from=builder /build/bench/suite.txt /app/bench/suite.txt

ENV OMP_NUM_THREADS=4

EXPOSE 8080

HEALTHCHECK --interval=10s --timeout=3s --retries=3 \
    CMD curl -f http://localhost:8080/healthz || exit 1

CMD ["./mancala_server", "8080"]