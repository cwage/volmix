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

# Extract package name from debian/control
PACKAGE_NAME=$(grep "^Source:" debian/control | cut -d' ' -f2)
if [ -z "$PACKAGE_NAME" ]; then
    echo "ERROR: Could not extract package name from debian/control"
    exit 1
fi

# Get maintainer info from environment or extract from existing changelog
MAINTAINER_LINE=$(grep "^ --" debian/changelog 2>/dev/null | head -1)
if [[ "$MAINTAINER_LINE" =~ ^\ \-\-\ ([^\<]+)\ \<([^\>]+)\> ]]; then
    MAINTAINER_NAME="${MAINTAINER_NAME:-${BASH_REMATCH[1]}}"
    MAINTAINER_EMAIL="${MAINTAINER_EMAIL:-${BASH_REMATCH[2]}}"
fi

# Fallback to defaults if extraction fails
MAINTAINER_NAME="${MAINTAINER_NAME:-Chris Wage}"
MAINTAINER_EMAIL="${MAINTAINER_EMAIL:-cwage@quietlife.net}"

# Update debian/changelog with current version
# Determine next revision number for this version
if [ -f debian/changelog ]; then
    # Extract all revision numbers for this version, sort, and get the highest
    LAST_REVISION=$(grep -E "^$PACKAGE_NAME \($VERSION-[0-9]+\)" debian/changelog | \
        sed -E "s/^$PACKAGE_NAME \($VERSION-([0-9]+)\).*/\1/" | sort -n | tail -1)
    if [ -n "$LAST_REVISION" ]; then
        NEXT_REVISION=$((LAST_REVISION + 1))
    else
        NEXT_REVISION=1
    fi
else
    NEXT_REVISION=1
fi
CHANGELOG_ENTRY="$PACKAGE_NAME ($VERSION-$NEXT_REVISION) unstable; urgency=medium

  * Release version $VERSION
  * See git log for detailed changes

 -- $MAINTAINER_NAME <$MAINTAINER_EMAIL> $(date -R)
"

# Create temporary changelog with new version
echo "$CHANGELOG_ENTRY" > debian/changelog.tmp
if [ -f debian/changelog ]; then
    echo "" >> debian/changelog.tmp
    cat debian/changelog >> debian/changelog.tmp
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