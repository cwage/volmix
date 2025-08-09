#!/bin/bash
# Build script for volmix Debian package using Docker

set -e

# Parse command line arguments
ARCHITECTURE="${1:-amd64}"
BUILD_TYPE="${2:-release}"

echo "Building volmix Debian package..."
echo "Architecture: $ARCHITECTURE"
echo "Build Type: $BUILD_TYPE"

# Get version from git tag or default to 1.0.0
if git describe --tags --exact-match 2>/dev/null; then
    VERSION=$(git describe --tags --exact-match | sed 's/^v//')
else
    VERSION="1.0.0"
fi
echo "Version: $VERSION"

# Update debian/changelog with current version
CHANGELOG_ENTRY="volmix ($VERSION-1) unstable; urgency=medium

  * Release version $VERSION
  * See git log for detailed changes

 -- Chris Wage <cwage@quietlife.net>  $(date -R)
"

# Create temporary changelog with new version
echo "$CHANGELOG_ENTRY" > debian/changelog.tmp
if [ -f debian/changelog ]; then
    echo "" >> debian/changelog.tmp
    tail -n +2 debian/changelog >> debian/changelog.tmp
fi
mv debian/changelog.tmp debian/changelog

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