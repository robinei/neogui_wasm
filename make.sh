set -e

mkdir -p web/build
tsc &

if [ ! -d builddir ]; then
    meson builddir
fi
ninja -C builddir &

emcc -g -O2 -flto -fno-rtti -fno-exceptions \
    -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_HARFBUZZ=1 \
    --js-library web/library.js \
    native/neogui.c native/text.c \
    -o web/build/neogui.js &

wait
