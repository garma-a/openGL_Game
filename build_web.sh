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

# Compile
# -O3: Optimization level (make it run fast)
# -s LEGACY_GL_EMULATION=1: Enable support for glBegin/glEnd (Immediate mode)
# -s USE_SDL=2: Use SDL2 port
# -s USE_SDL_MIXER=2: Use SDL2 Mixer port
# -s ALLOW_MEMORY_GROWTH=1: Allow game to use more RAM if needed
# --preload-file sounds: Pack the sounds folder into the game
# -o docs/index.html: Output file

emcc main.cpp -o docs/index.html \
    -O3 \
    -s LEGACY_GL_EMULATION=1 \
    -s USE_SDL=2 \
    -s USE_SDL_MIXER=2 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -lglut -lGL -lGLU \
    --preload-file sounds \
    --shell-file shell_minimal.html

echo "Build complete! Go to the 'docs' folder and run a server."

