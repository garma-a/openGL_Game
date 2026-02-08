#!/bin/bash

# Check if emcc is installed
if ! command -v emcc &> /dev/null
then
    echo "Error: Emscripten (emcc) is not found."
    echo "Please install it from https://emscripten.org/docs/getting_started/downloads.html"
    echo "And remember to run 'source ./emsdk_env.sh' before building."
    exit 1
fi

echo "Compiling for WebAssembly..."

# Create a build directory
mkdir -p docs

# Create a temporary directory with only the needed sound files for the web build.
# Use mp3 where available (much smaller than wav) to reduce download size.
SOUNDS_WEB=$(mktemp -d) || { echo "Error: Failed to create temporary directory"; exit 1; }
mkdir -p "$SOUNDS_WEB/sounds"
cp sounds/background_sound_track.mp3 "$SOUNDS_WEB/sounds/"
cp sounds/freeze.mp3 "$SOUNDS_WEB/sounds/"
cp sounds/level_up.mp3 "$SOUNDS_WEB/sounds/"
cp sounds/pickup.mp3 "$SOUNDS_WEB/sounds/"
cp sounds/shoot.wav "$SOUNDS_WEB/sounds/"
cp sounds/zombie_death.mp3 "$SOUNDS_WEB/sounds/"

# Compile
# -O3: Optimization level (make it run fast)
# -s LEGACY_GL_EMULATION=1: Enable support for glBegin/glEnd (Immediate mode)
# -s USE_SDL=2: Use SDL2 port
# -s USE_SDL_MIXER=2: Use SDL2 Mixer port
# -s ALLOW_MEMORY_GROWTH=1: Allow game to use more RAM if needed
# --preload-file: Pack only needed sound files (mp3 for smaller download)
# -o docs/index.html: Output file

emcc main.cpp -o docs/index.html \
    -O3 \
    -s LEGACY_GL_EMULATION=1 \
    -s USE_SDL=2 \
    -s USE_SDL_MIXER=2 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -lglut -lGL -lGLU \
    --preload-file "$SOUNDS_WEB/sounds@sounds" \
    --shell-file shell_minimal.html

# Clean up temporary directory
rm -rf "$SOUNDS_WEB"

# Ensure .nojekyll exists for GitHub Pages (prevents Jekyll from mangling binary files)
touch docs/.nojekyll

echo "Build complete! Go to the 'docs' folder and run a server."

