#!/bin/bash
# Build script for llm-npc voice chat application
# Run from MSYS2 UCRT64 terminal

set -e  # Exit on error

echo "=== Building llm-npc ==="

# Check we're in MSYS2 UCRT64 environment
if [[ "$MSYSTEM" != "UCRT64" ]]; then
    echo "Warning: This script is designed for MSYS2 UCRT64 environment"
    echo "Current environment: $MSYSTEM"
    echo "Run from MSYS2 UCRT64 terminal for best results"
fi

# Install dependencies if missing
echo "=== Checking dependencies ==="
PACKAGES=""

# Check for each required package
command -v gcc >/dev/null 2>&1 || PACKAGES="$PACKAGES mingw-w64-ucrt-x86_64-gcc"
command -v cmake >/dev/null 2>&1 || PACKAGES="$PACKAGES mingw-w64-ucrt-x86_64-cmake"
command -v ninja >/dev/null 2>&1 || PACKAGES="$PACKAGES mingw-w64-ucrt-x86_64-ninja"
command -v git >/dev/null 2>&1 || PACKAGES="$PACKAGES git"

# Check for SDL2 (library, not command)
if ! pkg-config --exists sdl2 2>/dev/null; then
    PACKAGES="$PACKAGES mingw-w64-ucrt-x86_64-SDL2"
fi

# Check for curl
if ! pkg-config --exists libcurl 2>/dev/null; then
    PACKAGES="$PACKAGES mingw-w64-ucrt-x86_64-curl"
fi

if [ -n "$PACKAGES" ]; then
    echo "Installing missing packages:$PACKAGES"
    pacman -S --needed --noconfirm $PACKAGES
else
    echo "All dependencies installed"
fi

# Check we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Run this script from the llm-npc root directory"
    exit 1
fi

# Initialize submodules if needed
if [ ! -f "whisper.cpp/CMakeLists.txt" ]; then
    echo "Initializing submodules..."
    git submodule update --init --recursive
fi

# Step 1: Build whisper.cpp
echo ""
echo "=== Step 1: Building whisper.cpp ==="
cd whisper.cpp
cmake -B build -G Ninja
cmake --build build
cd ..

# Download whisper model if not present
if [ ! -f "whisper.cpp/models/ggml-base.en.bin" ]; then
    echo ""
    echo "=== Downloading whisper model ==="
    cd whisper.cpp
    ./models/download-ggml-model.sh base.en
    cd ..
fi

# Download piper voice model if not present
if [ ! -f "models/en_US-lessac-medium.onnx" ]; then
    echo ""
    echo "=== Downloading piper voice model ==="
    mkdir -p models
    curl -L -o models/en_US-lessac-medium.onnx \
        https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
    curl -L -o models/en_US-lessac-medium.onnx.json \
        https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
fi

# Step 2: Build libpiper
echo ""
echo "=== Step 2: Building libpiper ==="
cd piper1-gpl/libpiper
cmake -B build -G Ninja -DCMAKE_INSTALL_PREFIX=install
cmake --build build
cmake --install build
cd ../..

# Step 3: Build voice_chat
echo ""
echo "=== Step 3: Building voice_chat ==="
cmake -B build -G Ninja
cmake --build build

# Step 4: Copy DLLs
echo ""
echo "=== Step 4: Copying DLLs ==="

# Core libraries
cp piper1-gpl/libpiper/install/libpiper.dll build/
cp piper1-gpl/libpiper/install/lib/onnxruntime.dll build/

# Runtime DLLs
cp /ucrt64/bin/libgomp-1.dll build/
cp /ucrt64/bin/libstdc++-6.dll build/
cp /ucrt64/bin/libgcc_s_seh-1.dll build/
cp /ucrt64/bin/libwinpthread-1.dll build/
cp /ucrt64/bin/SDL2.dll build/
cp /ucrt64/bin/libcurl-4.dll build/

# Curl dependencies
cp /ucrt64/bin/zlib1.dll build/
cp /ucrt64/bin/libbrotlidec.dll build/
cp /ucrt64/bin/libbrotlicommon.dll build/
cp /ucrt64/bin/libidn2-0.dll build/
cp /ucrt64/bin/libnghttp2-14.dll build/
cp /ucrt64/bin/libpsl-5.dll build/
cp /ucrt64/bin/libssh2-1.dll build/
cp /ucrt64/bin/libssl-3-x64.dll build/
cp /ucrt64/bin/libcrypto-3-x64.dll build/
cp /ucrt64/bin/libzstd.dll build/
cp /ucrt64/bin/libunistring-5.dll build/
cp /ucrt64/bin/libiconv-2.dll build/
cp /ucrt64/bin/libintl-8.dll build/
cp /ucrt64/bin/libnghttp3-9.dll build/ 2>/dev/null || true
cp /ucrt64/bin/libngtcp2-16.dll build/ 2>/dev/null || true
cp /ucrt64/bin/libngtcp2_crypto_ossl-0.dll build/ 2>/dev/null || true

echo ""
echo "=== Build complete! ==="
echo ""
echo "To run:"
echo "  export OPENROUTER_API_KEY=your-key-here"
echo "  ./build/voice_chat.exe -wm whisper.cpp/models/ggml-base.en.bin -pm models/en_US-lessac-medium.onnx -ed piper1-gpl/libpiper/install/espeak-ng-data"
