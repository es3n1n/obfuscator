FROM ubuntu:22.04

# Set non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary build tools and add LLVM repository for newer clang
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 17 all \
    && rm -rf /var/lib/apt/lists/* \
    && rm llvm.sh

# Set clang as the default compiler to ensure C++23 support
ENV CC=clang-17
ENV CXX=clang++-17

# Create app directory
WORKDIR /app

# Clone the repository (alternatively, you can COPY from local or mount at runtime)
# For building from a local copy, comment this line and use COPY . /app instead
RUN git clone --recurse-submodules https://github.com/es3n1n/obfuscator.git .

# Build the obfuscator
RUN cmake -B build -DOBFUSCATOR_BUILD_TESTS=0 -DCMAKE_CXX_FLAGS="-stdlib=libc++" && \
    cmake --build build --config Release

# Add the obfuscator to PATH
ENV PATH="/app/build/src:${PATH}"

# Create a directory for input/output files
RUN mkdir -p /data
WORKDIR /data

# Command that will be run when the container starts
ENTRYPOINT ["obfuscator"]

# Default arguments (override with docker run)
CMD ["--help"]