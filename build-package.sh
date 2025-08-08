#!/bin/bash
# Build script for volume Debian package using Docker

set -e

# Parse command line arguments
ARCHITECTURE="${1:-amd64}"
BUILD_TYPE="${2:-release}"

echo "Building volume Debian package..."
echo "Architecture: $ARCHITECTURE"
echo "Build Type: $BUILD_TYPE"

# Build the Docker image
echo "Building Docker image..."
docker build -t volume-builder .

# Run the build with volume mount to output packages to current directory
echo "Running package build..."
docker run --rm \
    -v "$(pwd):/src" \
    -w /src \
    volume-builder \
    bash -c "dpkg-buildpackage -us -uc && cp ../*.deb . 2>/dev/null"

# Check if any .deb files were generated
debs=$(find_debs)
if [ -n "$debs" ]; then
    echo "Package build complete!"
    echo "Generated files:"
    ls -la *.deb
else
    echo "ERROR: No .deb package found. Build may have failed."
    exit 1
fi