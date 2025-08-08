#!/bin/bash
# Build script for volmix Debian package using Docker

set -e

# Parse command line arguments
ARCHITECTURE="${1:-amd64}"
BUILD_TYPE="${2:-release}"

echo "Building volmix Debian package..."
echo "Architecture: $ARCHITECTURE"
echo "Build Type: $BUILD_TYPE"

# Build the Docker image with current user's UID/GID
echo "Building Docker image..."
docker build \
    --build-arg USER_ID=$(id -u) \
    --build-arg GROUP_ID=$(id -g) \
    -t volmix-builder .

# Run the build with volume mount to output packages to current directory
echo "Running package build..."
docker run --rm \
    -v "$(pwd):/src" \
    -w /home/builder/build \
    volmix-builder \
    bash -c "cp -r /src/. . && dpkg-buildpackage -us -uc && cp ../*.deb /src/ 2>/dev/null"

# Check if any .deb files were generated
debs=$(find . -maxdepth 1 -type f -name '*.deb')
if [ -n "$debs" ]; then
    echo "Package build complete!"
    echo "Generated files:"
    ls -la *.deb
else
    echo "ERROR: No .deb package found. Build may have failed."
    exit 1
fi