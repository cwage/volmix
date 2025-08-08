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
    dpkg-buildpackage -us -uc

echo "Package build complete!"
echo "Generated files:"
ls -la *.deb *.changes *.buildinfo 2>/dev/null || echo "No package files found in current directory"