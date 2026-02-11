# Installation Guide

This document provides detailed instructions for building and installing eRaft on macOS and Linux systems.

## Table of Contents

- [Prerequisites](#prerequisites)
- [macOS Installation](#macos-installation)
- [Linux Installation](#linux-installation)
- [Building from Source](#building-from-source)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Software

- **Go**: Version 1.24 or higher
- **Protocol Buffers (protoc)**: For generating gRPC code
- **RocksDB**: C++ library for storage backend
- **Git**: For cloning the repository

### Optional Software

- **Make**: For using Makefile targets
- **Docker**: For containerized deployment

## macOS Installation

### Step 1: Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Step 2: Install Required Dependencies

```bash
# Install Go
brew install go

# Install Protocol Buffers
brew install protobuf

# Install RocksDB (required for storage backend)
brew install rocksdb

# Install Make (optional)
brew install make
```

### Step 3: Set Environment Variables

Add the following to your `~/.zshrc` or `~/.bash_profile`:

```bash
# RocksDB environment variables for CGO
export CGO_CFLAGS="-I$(brew --prefix rocksdb)/include"
export CGO_LDFLAGS="-L$(brew --prefix rocksdb)/lib -lrocksdb"

# Go environment
export PATH=$PATH:$(go env GOPATH)/bin
```

Then reload your shell configuration:

```bash
source ~/.zshrc  # or source ~/.bash_profile
```

## Linux Installation

### Ubuntu/Debian

#### Step 1: Install System Dependencies

```bash
# Update package list
sudo apt-get update

# Install build essentials
sudo apt-get install -y build-essential git wget

# Install RocksDB dependencies
sudo apt-get install -y libgflags-dev libsnappy-dev zlib1g-dev \
    libbz2-dev liblz4-dev libzstd-dev

# Install Protocol Buffers
sudo apt-get install -y protobuf-compiler
```

#### Step 2: Install RocksDB from Source

```bash
# Download and build RocksDB
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
git checkout v9.9.3  # Use a stable version

# Build shared library
make shared_lib -j$(nproc)

# Install to system
sudo make install-shared

# Update library cache
sudo ldconfig
```

#### Step 3: Install Go

```bash
# Download Go (adjust version as needed)
wget https://go.dev/dl/go1.24.0.linux-amd64.tar.gz

# Remove old Go installation (if exists)
sudo rm -rf /usr/local/go

# Extract new version
sudo tar -C /usr/local -xzf go1.24.0.linux-amd64.tar.gz

# Add to PATH
echo 'export PATH=$PATH:/usr/local/go/bin' >> ~/.bashrc
source ~/.bashrc
```

#### Step 4: Set Environment Variables

Add to your `~/.bashrc` or `~/.profile`:

```bash
# RocksDB environment variables for CGO
export CGO_CFLAGS="-I/usr/local/include"
export CGO_LDFLAGS="-L/usr/local/lib -lrocksdb"

# Go environment
export PATH=$PATH:$(go env GOPATH)/bin
```

Reload configuration:

```bash
source ~/.bashrc
```

### CentOS/RHEL/Fedora

#### Step 1: Install System Dependencies

```bash
# For CentOS/RHEL 8+
sudo dnf install -y gcc gcc-c++ make git wget

# Install RocksDB dependencies
sudo dnf install -y gflags-devel snappy-devel zlib-devel \
    bzip2-devel lz4-devel libzstd-devel

# Install Protocol Buffers
sudo dnf install -y protobuf-compiler
```

#### Step 2-4: Same as Ubuntu

Follow Steps 2-4 from the Ubuntu/Debian section above.

## Building from Source

### Step 1: Clone the Repository

```bash
git clone https://github.com/eraft-io/eraft.git
cd eraft
```

### Step 2: Download Go Dependencies

```bash
go mod download
```

### Step 3: Build the Project

#### Option A: Using Make (Recommended)

```bash
# Build all components
make build

# Build specific components
make build-kvraft      # Build KV Raft components
make build-shardkv     # Build ShardKV components
make build-shardctrler # Build Shard Controller components
make build-dashboard   # Build Dashboard
```

#### Option B: Using Go Commands

```bash
# Build all binaries
export CGO_CFLAGS="-I$(brew --prefix rocksdb)/include"  # macOS
export CGO_LDFLAGS="-L$(brew --prefix rocksdb)/lib -lrocksdb"  # macOS

# Linux users should use:
# export CGO_CFLAGS="-I/usr/local/include"
# export CGO_LDFLAGS="-L/usr/local/lib -lrocksdb"

# Build all components
go build -o output/kvserver ./cmd/kvserver
go build -o output/kvclient ./cmd/kvclient
go build -o output/shardkvserver ./cmd/shardkvserver
go build -o output/shardkvclient ./cmd/shardkvclient
go build -o output/shardctrlerserver ./cmd/shardctrlerserver
go build -o output/shardctrlerclient ./cmd/shardctrlerclient
```

### Step 4: Build Protocol Buffer Files (if needed)

```bash
# Regenerate gRPC code from proto files
./proto_gen.sh
```

## Verification

### Check Binaries

```bash
# List built binaries
ls -la output/

# Expected output:
# - kvserver
# - kvclient
# - shardkvserver
# - shardkvclient
# - shardctrlerserver
# - shardctrlerclient
# - dashboard-server (if built)
```

### Run Tests

```bash
# Run all tests
make test

# Or using go directly
go test ./...
```

### Quick Start Test

```bash
# Start a simple KV server
./output/kvserver -id 0 -cluster "localhost:8001,localhost:8002,localhost:8003" -db "data/kv0"

# In another terminal, test with client
./output/kvclient put mykey myvalue
./output/kvclient get mykey
```

## Troubleshooting

### Common Issues

#### 1. RocksDB Header Not Found

**Error**: `'rocksdb/c.h' file not found`

**Solution**:
- macOS: `brew install rocksdb`
- Linux: Build and install RocksDB from source (see Linux section)

#### 2. CGO Compilation Errors

**Error**: `undefined reference to 'rocksdb_*'`

**Solution**: Ensure environment variables are set:
```bash
export CGO_CFLAGS="-I/path/to/rocksdb/include"
export CGO_LDFLAGS="-L/path/to/rocksdb/lib -lrocksdb"
```

#### 3. Library Not Found at Runtime

**Error**: `cannot find -lrocksdb`

**Solution**:
- macOS: Add to `~/.zshrc`: `export DYLD_LIBRARY_PATH=$(brew --prefix rocksdb)/lib:$DYLD_LIBRARY_PATH`
- Linux: Run `sudo ldconfig` after installing RocksDB

#### 4. Permission Denied

**Error**: `permission denied` when running binaries

**Solution**:
```bash
chmod +x output/*
```

#### 5. Go Module Issues

**Error**: `go: module not found`

**Solution**:
```bash
go mod tidy
go mod download
```

### Platform-Specific Notes

#### macOS

- If using Apple Silicon (M1/M2/M3), ensure Homebrew is installed in `/opt/homebrew`
- You may need to install Xcode Command Line Tools: `xcode-select --install`

#### Linux

- Ensure `pkg-config` is installed for proper library detection
- Some distributions may require manual LD_LIBRARY_PATH configuration

## Docker Build (Alternative)

If you prefer containerized builds:

```bash
# Build Docker image
docker build -t eraft:latest .

# Run container
docker run -it --rm eraft:latest
```

## Next Steps

After successful installation:

1. Read the [Quick Start Guide](README.md#quick-start-guide)
2. Explore the [Wiki Documentation](wiki/)
3. Try running the [examples](examples/)

## Support

For additional help:

- Check [GitHub Issues](https://github.com/eraft-io/eraft/issues)
- Review [Wiki Documentation](wiki/)
- Join our community discussions

---

**Note**: This installation guide assumes a clean environment. If you have existing Go or RocksDB installations, you may need to adjust paths accordingly.
