#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL_FontCache.h"
#include "soloud_c.h"

#include "renderer.h"
#include "utils.h"

#define FRAMES_PER_SECOND 60

#define GetKey(n) keyboard_current[n]
#define GetKeyDown(n) (keyboard_current[n] && !keyboard_last[n])
#define GetKeyUp(n) (!keyboard_current[n] && keyboard_last[n])

//Internal resolution, used in rendering. Defined by SCREEN_SCALE
int GAME_SCREEN_WIDTH = 640;
int GAME_SCREEN_HEIGHT = 360;

//Game window resolution
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;

int SCREEN_SCALE = 1;

int Exit = 0;
int ErrorOcurred = 0;
char *fpscounter;

FC_Font* font = NULL;
Soloud *soloud = NULL;

//Time between frames
double deltaTime = 0;

//Array with the keyboard state
const Uint8 *keyboard_current = NULL;
Uint8 *keyboard_last;
SDL_Event event;

extern Vector3 cameraRotation;

void InputUpdate();
void GameUpdate();

Model cube;

int main(int argc, char *argv[]){
	unsigned int frameTicks;
	unsigned int mstime = 0;

	SDL_Renderer * renderer = NULL;
	SDL_Window* window = NULL;	
	SDL_Texture * render = NULL;
	
	//General initializations

	srand( (unsigned)time(NULL) );
	
	soloud = Soloud_create();
	if(Soloud_initEx(soloud,SOLOUD_CLIP_ROUNDOFF | SOLOUD_ENABLE_VISUALIZATION, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO)<0){
		printf("SoLoud could not initialize! \n");
		ErrorOcurred = 1; goto EndProgram;
	}
    
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
        printf("SDL could not initialize! SLD_Error: %s\n", SDL_GetError());
		ErrorOcurred = 1; goto EndProgram;
	}
	if( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) )
	{
		printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
		ErrorOcurred = 1; goto EndProgram;
	}
	window = SDL_CreateWindow( "Retro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(window == NULL){
		printf("Window could not be created! SDL_Error %s\n", SDL_GetError() );
		ErrorOcurred = 1; goto EndProgram;
	}	
	
	//Define internal resolution
	GAME_SCREEN_WIDTH = SCREEN_WIDTH/SCREEN_SCALE;
	GAME_SCREEN_HEIGHT = SCREEN_HEIGHT/SCREEN_SCALE;

	//Initialize the renderer and the render texture
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	render = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
	SDL_RenderSetLogicalSize(renderer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_ADD);
	//Define the pointer to the texture array
	Pixel *pix;
	int pitch = GAME_SCREEN_WIDTH * sizeof(unsigned int);

	//Initialize frame counter string
	fpscounter = (char*)calloc(50,sizeof(char));

	//Initialize FPS counter
	InitFPS();
	Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    
    //Initialize keyboard array
	keyboard_last = (Uint8 *)calloc(284,sizeof(Uint8));
	keyboard_current = SDL_GetKeyboardState(NULL);

	//Initialize font
	font = FC_CreateFont();  
	if(!FC_LoadFont(font, renderer, "Visitor.ttf",18, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL)){
        printf("Font: Error loading font!\n");
	}

	if(!InitRenderer(renderer)){
		printf("Error initializing renderer!\n");
		ErrorOcurred = 1; goto EndProgram;
	}

	//Sun Texture
	SDL_Surface * sunSurf = IMG_Load("Textures/Sun.png");
	if(sunSurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * sunTex = SDL_CreateTextureFromSurface(renderer, sunSurf);
	int sunW, sunH;
	SDL_QueryTexture(sunTex, NULL, NULL, &sunW, &sunH);
	float sunRatio = sunH/(float)sunW;
	SDL_Rect texr; texr.x = GAME_SCREEN_WIDTH/2.67; texr.y = GAME_SCREEN_HEIGHT/4.5f; texr.w = GAME_SCREEN_WIDTH/4; texr.h = (GAME_SCREEN_WIDTH/4)*sunRatio; 
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	//Sky texture
	SDL_Surface * skySurf = IMG_Load("Textures/Sky.png");
	if(skySurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * skyTex = SDL_CreateTextureFromSurface(renderer, skySurf);
	int skyW, skyH;
	SDL_QueryTexture(skyTex, NULL, NULL, &skyW, &skyH);
	float skyRatio = skyH/(float)skyW;
	SDL_Rect texSky; texSky.x = 0; texSky.y = 0; texSky.w = GAME_SCREEN_WIDTH; texSky.h = GAME_SCREEN_WIDTH*skyRatio; 

	//Vignette Texture
	SDL_Surface * vigSurf = IMG_Load("Textures/Vignette.png");
	if(vigSurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * vigTex = SDL_CreateTextureFromSurface(renderer, vigSurf);
	int vigW, vigH;
	SDL_QueryTexture(vigTex, NULL, NULL, &vigW, &vigH);
	float vigRatio = vigH/(float)vigW;
	SDL_Rect texVig; texVig.x = 0; texVig.y = 0; texVig.w = GAME_SCREEN_WIDTH; texVig.h = GAME_SCREEN_WIDTH*vigRatio;
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	cube = LoadModel("Models/Plane.txt");
	cube.color = (Pixel){100,30,255,255};

	while (!Exit)
	{
		//Ms tick time
		frameTicks = SDL_GetTicks();
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST)*1000 / SDL_GetPerformanceFrequency() )*0.001;

		InputUpdate();
		GameUpdate();
		
		//Locks texture to manual modification
		SDL_LockTexture(render, NULL, (void**)&pix, &pitch);
			UpdateScreenPointer(pix);
			ClearScreen();
			RenderModel(&cube);
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &texSky);
		texr.x = cameraRotation.y*20 + GAME_SCREEN_WIDTH/2.67;
		texr.y = GAME_SCREEN_HEIGHT/4.5f - cameraRotation.x*18;
		
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &texVig);
		SDL_RenderCopy(renderer, sunTex, NULL, &texr);
		
        FC_DrawAlign(font, renderer, GAME_SCREEN_WIDTH,0,FC_ALIGN_RIGHT, "%4.2f :FPS\n%3d : MS\n%5.4lf : DT", GetFPS(), mstime, deltaTime);

		SDL_RenderPresent(renderer);

		//Calculates the time it took to process this iteration
		mstime = SDL_GetTicks()-frameTicks;
		ProcessFPS();
		
		//Waits for at least 60 frames to be elapsed
		while( SDL_GetTicks()-frameTicks <  (1000/FRAMES_PER_SECOND) ){ }
	}
	
	//End of the program
	EndProgram:
	free(fpscounter);
	free(keyboard_last);
	
	FreeRenderer();
	FreeModel(&cube);

	if(soloud!=NULL){
		Soloud_deinit(soloud);
		Soloud_destroy(soloud);
	}

	if(font!=NULL)
		FC_FreeFont(font);
			
	if(renderer!=NULL)
		SDL_DestroyRenderer(renderer);

    if(window!=NULL)
		SDL_DestroyWindow( window );

	if(SDL_WasInit(SDL_INIT_EVERYTHING)!=0)
    	SDL_Quit();

	if(ErrorOcurred)
		system("pause");
    return 0;
}

void InputUpdate(){
    //Updates keyboard array and manages events
    if(keyboard_current!=NULL){
        memcpy(keyboard_last,keyboard_current,284*sizeof(Uint8));
    }
    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
            case SDL_QUIT:
                Exit = 1;
                break;
        }
    }
}

void GameUpdate(){
		
	//Camera movement
	if (GetKey(SDL_SCANCODE_W))
	{
		MoveCamera((Vector3){0,0,-50});
	}
	else if (GetKey(SDL_SCANCODE_S))
	{
		MoveCamera((Vector3){0,0,100});
	}
	if (GetKey(SDL_SCANCODE_D))
	{
		MoveCamera((Vector3){-50,0,0});
	}
	else if (GetKey(SDL_SCANCODE_A))
	{
		MoveCamera((Vector3){50,0,0});
	}
	if (GetKey(SDL_SCANCODE_E))
	{
		MoveCamera((Vector3){0,10,0});
	}
	else if (GetKey(SDL_SCANCODE_Q))
	{
		MoveCamera((Vector3){0,-10,0});
	}


	if (GetKey(SDL_SCANCODE_U))
	{
		RotateCamera((Vector3){0,0,-20});
	}
	else if (GetKey(SDL_SCANCODE_J))
	{
		RotateCamera((Vector3){0,0,20});
	}
	if (GetKey(SDL_SCANCODE_K))
	{
		RotateCamera((Vector3){20,0,0});
	}
	else if (GetKey(SDL_SCANCODE_H))
	{
		RotateCamera((Vector3){-20,0,0});
	}
	if (GetKey(SDL_SCANCODE_I))
	{
		RotateCamera((Vector3){0,20,0});
	}
	else if (GetKey(SDL_SCANCODE_Y))
	{
		RotateCamera((Vector3){0,-20,0});
	}

	if (GetKey(SDL_SCANCODE_ESCAPE))
	{
		Exit = 1;
	}
}