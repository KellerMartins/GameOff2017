#OBJS specifies which files to compile as part of the project
OBJS = main.c renderer.c utils.c SDL_FontCache.c GameLogic.c
all : $(OBJS)
	emcc $(OBJS) -o build/sunsetRun.js -s NO_EXIT_RUNTIME=0 -s WASM=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -O3 --emrun --preload-file Assets -s ALLOW_MEMORY_GROWTH=1 -s "BINARYEN_TRAP_MODE='clamp'"