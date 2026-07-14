#!/bin/bash
# Quick-start build script for Jangolizer Plugin

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Jangolizer Plugin Builder ===${NC}"
echo ""

# Parse command line arguments
BUILD_TYPE="${1:-default}"
CMAKE_PRESET="${BUILD_TYPE}"

case "$BUILD_TYPE" in
    desktop|default)
        CMAKE_PRESET="default"
        echo -e "${YELLOW}Building: Desktop with GUI (Ardour/Linux)${NC}"
        ;;
    release)
        CMAKE_PRESET="release"
        echo -e "${YELLOW}Building: Release (Desktop, Optimized)${NC}"
        ;;
    elk|elk-headless)
        CMAKE_PRESET="elk-headless"
        echo -e "${YELLOW}Building: Elk Audio OS (Headless)${NC}"
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build directories...${NC}"
        rm -rf cmake-build cmake-build-debug cmake-build-release cmake-build-elk
        echo -e "${GREEN}Clean complete!${NC}"
        exit 0
        ;;
    *)
        echo "Usage: $0 [desktop|release|elk|clean]"
        echo ""
        echo "Options:"
        echo "  desktop     - Build with GUI for desktop (default)"
        echo "  release     - Build optimized release for desktop"
        echo "  elk         - Build headless for Elk Audio OS"
        echo "  clean       - Remove all build directories"
        exit 1
        ;;
esac

echo ""

# Determine build directory
BUILD_DIR="cmake-build"
case "$CMAKE_PRESET" in
    default)
        BUILD_DIR="cmake-build"
        ;;
    release)
        BUILD_DIR="cmake-build-release"
        ;;
    elk-headless)
        BUILD_DIR="cmake-build-elk"
        ;;
esac

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creating build directory: $BUILD_DIR${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Configure
echo -e "${YELLOW}Running CMake configuration...${NC}"
cd "$BUILD_DIR"
cmake .. --preset "$CMAKE_PRESET"
echo -e "${GREEN}Configuration complete!${NC}"
echo ""

# Build
echo -e "${YELLOW}Building plugin...${NC}"
cmake --build . --config Debug
echo -e "${GREEN}Build complete!${NC}"
echo ""

# Print artifact location
ARTIFACT_DIR="$BUILD_DIR/JangolizerPlugin_artefacts/Debug"
if [ -d "$ARTIFACT_DIR" ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "Artifacts located in: ${YELLOW}$ARTIFACT_DIR${NC}"
    echo ""
    ls -lh "$ARTIFACT_DIR"/
else
    echo -e "${YELLOW}⚠ Artifacts directory not found yet. First build may need additional time.${NC}"
fi
