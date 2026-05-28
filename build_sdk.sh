#!/bin/bash
set -e

echo "Building Standard VFX Engine SDK..."

# Clean old build
rm -rf build
mkdir -p build
cd build

# Configure CMake with installation prefix pointing to sdk_output
cmake -DCMAKE_INSTALL_PREFIX=../sdk_release -DVFX_BUILD_TESTS=OFF -DVFX_BUILD_EXAMPLES=OFF ..

# Build
make -j$(nproc)

# Install into standard SDK structure (include, lib, bin)
make install

cd ..
echo ""
echo "=== Standard SDK output generated in sdk_release/ ==="
tree sdk_release || ls -laR sdk_release
