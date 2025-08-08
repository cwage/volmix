#!/bin/bash
# Build script for volume Debian package using Docker

set -e

echo "Building volume Debian package..."

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
if ls *.deb 1> /dev/null 2>&1; then
    echo "Package build complete!"
    echo "Generated files:"
    ls -la *.deb
else
    echo "ERROR: No .deb package found. Build may have failed."
    exit 1
fi