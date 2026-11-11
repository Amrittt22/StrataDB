
FROM debian:bookworm-slim

# Install C++ compiler, CMake and dependencies needed by FetchContent
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    curl \
    unzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy StrataDB source code
COPY . .

# Configure and compile the HTTP server
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target stratadb_server -j2

# Railway will provide PORT
ENV HOST=0.0.0.0

EXPOSE 8080

# Use Railway's assigned port and persistent data directory
CMD ["sh", "-c", "./build/stratadb_server /data ${PORT:-8080} 1000"]
