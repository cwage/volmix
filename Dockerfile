# Dockerfile for building volume Debian package
# This provides a clean environment with all necessary build dependencies

FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package list and install build dependencies
RUN apt-get update && apt-get install -y \
    # Debian packaging tools
    debhelper \
    dh-autoreconf \
    devscripts \
    build-essential \
    fakeroot \
    # Project build dependencies  
    pkg-config \
    libgtk-3-dev \
    libpulse-dev \
    libglib2.0-dev \
    # Autotools (needed for autogen.sh)
    autoconf \
    automake \
    autotools-dev \
    # Utilities
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /src

# Default command for interactive use
CMD ["/bin/bash"]