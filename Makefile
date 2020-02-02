CC = gcc

OBJS = $(wildcard src/*.c) third_party/SDL_FontCache/src/SDL_FontCache.c

INCLUDE = -I third_party/SDL_FontCache/include/
native: INCLUDE += -I D:/DevelopmentLibs/SDL2/include
				LIBS = -L D:/DevelopmentLibs/SDL2/lib

				ifeq ($(OS),Windows_NT)
					ASSETS = build\win32\Assets
					OUT = build\win32\sunsetrun
					LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -m32
				
				else
					ASSETS = build/linux/Assets
					OUT = build/linux/sunsetrun
					LINKER_FLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm
				endif

all: native

wasm : $(OBJS)
	emcc $(OBJS) $(INCLUDE) -o build/wasm/sunsetRun.js -s NO_EXIT_RUNTIME=0 -s WASM=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -O3 -Wall --emrun --preload-file Assets -s ALLOW_MEMORY_GROWTH=1

native: $(ASSETS)
	$(CC) $(OBJS) $(INCLUDE) $(LIBS) $(LINKER_FLAGS) -O3 -Wall -o $(OUT)

$(ASSETS):
ifeq ($(OS),Windows_NT)
	XCOPY /e /i /y Assets $(ASSETS)
else
	cp -r Assets $(ASSETS)
endif

.PHONY: build/%/Assets

clean:
ifeq ($(OS),Windows_NT)
	-rd /s /q $(ASSETS)
	-del /q $(OUT).exe
	-del build\wasm\sunsetRun.js
	-del build\wasm\sunsetRun.data
	-del build\wasm\sunsetRun.wasm
else
	-rm -f -r $(ASSETS)
	-rm -f $(OUT)
	-rm -f build/wasm/sunsetRun.js
	-rm -f build/wasm/sunsetRun.data
	-rm -f build/wasm/sunsetRun.wasm
endif