PD = D:
#CC specifies which compiler we're using
CC = gcc

#OBJS specifies which files to compile as part of the project
OBJS = main.c renderer.c utils.c SDL_FontCache.c GameLogic.c

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS_W = -I $(PD)\SDL2\SDL2_MinGW_32Bits\include -I $(PD)\SoLoud\include 
INCLUDE_PATHS_L = 
#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS_W = -L $(PD)\SDL2\SDL2_MinGW_32Bits\lib -L $(PD)\SoLoud\lib
LIBRARY_PATHS_L = 
#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
# -Wl,-subsystem,windows
# -fopenmp enables openmp support
COMPILER_FLAGS = -Wall -ffast-math -O3 

#LINKER_FLAGS specifies the libraries we're linking against -lglew32  -mwindows
LINKER_FLAGS_W = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf $(PD)\SoLoud\lib\soloud_x86.lib
LINKER_FLAGS_L = -lSDL2 -lSDL2_image -lSDL2_ttf -lm 
#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = Retro

#This is the target that compiles our executable
ifeq ($(OS),Windows_NT)
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS_W) $(LIBRARY_PATHS_W) $(COMPILER_FLAGS) $(LINKER_FLAGS_W) -o $(OBJ_NAME)
else
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS_L) $(LIBRARY_PATHS_L) $(COMPILER_FLAGS) $(LINKER_FLAGS_L) -o $(OBJ_NAME)
endif
