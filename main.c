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
#include "GameLogic.h"

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

unsigned SCREEN_SCALE = 1;
unsigned BLOOMS1_DOWNSCALE = 2;
unsigned BLOOMS2_DOWNSCALE = 4;

unsigned FOV = 70;
int BLOOM_ENABLED = 1;

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
extern Vector3 cameraForward;
extern Vector3 cameraUp;
extern Vector3 cameraRight;

void InputUpdate();
void GameUpdate();
int main(int argc, char *argv[]){
	unsigned int frameTicks;
	unsigned int mstime = 0;

	SDL_Renderer * renderer = NULL;
	SDL_Window* window = NULL;	
	SDL_Texture * render = NULL;
	SDL_Texture * bloomStep1 = NULL;
	SDL_Texture * bloomStep2 = NULL;

	//General initializations

	//Random from start time
	srand( (unsigned)time(NULL) );
	
	//Initializing SoLoud sound engine
	soloud = Soloud_create();
	if(Soloud_initEx(soloud,SOLOUD_CLIP_ROUNDOFF | SOLOUD_ENABLE_VISUALIZATION, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO)<0){
		printf("SoLoud could not initialize! \n");
		ErrorOcurred = 1; goto EndProgram;
	}
	
	//Initializing SDL
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
        printf("SDL could not initialize! SLD_Error: %s\n", SDL_GetError());
		ErrorOcurred = 1; goto EndProgram;
	}
	//Initializing SDL_Image
	if( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) )
	{
		printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
		ErrorOcurred = 1; goto EndProgram;
	}
	//Creating window
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
	
	//Creating the bloom textures
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"best");
	bloomStep1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS1_DOWNSCALE);
	bloomStep2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS2_DOWNSCALE);
	SDL_SetTextureBlendMode(bloomStep1,SDL_BLENDMODE_ADD);
	SDL_SetTextureBlendMode(bloomStep2,SDL_BLENDMODE_ADD);
	
	//Define the pointer to the textures pixel array
	Pixel *renderPix;
	int renderPitch = GAME_SCREEN_WIDTH * sizeof(unsigned int);

	Pixel *bloomS1Pix;
	int bloomS1Pitch = (GAME_SCREEN_WIDTH/4 ) * sizeof(unsigned int);
	Pixel *bloomS2Pix;
	int bloomS2Pitch = (GAME_SCREEN_WIDTH/16 ) * sizeof(unsigned int);

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
	SDL_FreeSurface(sunSurf);
	int sunW, sunH;
	SDL_QueryTexture(sunTex, NULL, NULL, &sunW, &sunH);
	float sunRatio = sunH/(float)sunW;
	SDL_Rect rectr; rectr.x = GAME_SCREEN_WIDTH/2.67; rectr.y = GAME_SCREEN_HEIGHT/4.5f; rectr.w = GAME_SCREEN_WIDTH/4; rectr.h = (GAME_SCREEN_WIDTH/4)*sunRatio; 
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	//How much the sun must move as the camera rotates
	Vector3 sunRot = {(150000/(FOV*FOV)),(120000/(FOV*FOV)),0};

	//Sky texture
	SDL_Surface * skySurf = IMG_Load("Textures/Sky.png");
	if(skySurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * skyTex = SDL_CreateTextureFromSurface(renderer, skySurf);
	SDL_FreeSurface(skySurf);
	int skyW, skyH;
	SDL_QueryTexture(skyTex, NULL, NULL, &skyW, &skyH);
	float skyRatio = skyH/(float)skyW;
	SDL_Rect rectSky; rectSky.x = 0; rectSky.y = 0; rectSky.w = GAME_SCREEN_WIDTH; rectSky.h = GAME_SCREEN_WIDTH*skyRatio; 

	//Vignette Texture
	SDL_Surface * vigSurf = IMG_Load("Textures/Vignette.png");
	if(vigSurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * vigTex = SDL_CreateTextureFromSurface(renderer, vigSurf);
	SDL_FreeSurface(vigSurf);
	int vigW, vigH;
	SDL_QueryTexture(vigTex, NULL, NULL, &vigW, &vigH);
	float vigRatio = vigH/(float)vigW;
	SDL_Rect rectVig; rectVig.x = 0; rectVig.y = 0; rectVig.w = GAME_SCREEN_WIDTH; rectVig.h = GAME_SCREEN_WIDTH*vigRatio;
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	//Game title Texture
	//(Disabled for now)
	/*SDL_Surface * titleSurf = IMG_Load("Textures/Title.png");
	if(titleSurf == NULL){
		printf("Error opening image!\n");
	}
	SDL_Texture * titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
	SDL_FreeSurface(titleSurf);
	int titleW, titleH;
	SDL_QueryTexture(titleTex, NULL, NULL, &titleW, &titleH);
	float titleRatio = titleH/(float)titleW;
	SDL_Rect rectTitle; rectTitle.x = GAME_SCREEN_WIDTH/12; rectTitle.y = 0; rectTitle.w = GAME_SCREEN_WIDTH/1.2; rectTitle.h = GAME_SCREEN_WIDTH/1.2 * titleRatio;
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	Model Play = LoadModel("Models/Play.txt");
	Play.color = (Pixel){100,30,255,255};
	Model Options = LoadModel("Models/Options.txt");
	Options.color = (Pixel){100,30,255,255};
	Model ExitModel = LoadModel("Models/Exit.txt");
	ExitModel.color = (Pixel){100,30,255,255};


	//TransformCamera((Vector3){0,4.02,22.5},(Vector3){-2.16,1.3,0});
	*/

	Model Track = LoadModel("Models/TestTrack.txt");
	InitCars();
	//Game loop
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
		SDL_LockTexture(render, NULL, (void**)&renderPix, &renderPitch);
			UpdateScreenPointer(renderPix);
			ClearScreen();
			
			RenderModel(&Track);
			RenderCars();

			if(BLOOM_ENABLED){
				//Process first bloom pass
				SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
					RenderBloom(bloomS1Pix,BLOOMS1_DOWNSCALE);
				SDL_UnlockTexture(bloomStep1);

				//Process second bloom pass
				SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
					RenderBloom(bloomS2Pix,BLOOMS2_DOWNSCALE);
				SDL_UnlockTexture(bloomStep2);
			}
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

		rectr.x = cameraRotation.y*sunRot.x + GAME_SCREEN_WIDTH/2.67;
		rectr.y = GAME_SCREEN_HEIGHT/4.5f - cameraRotation.x*sunRot.y;
		SDL_RenderCopy(renderer, sunTex, NULL, &rectr);

		if(BLOOM_ENABLED){
			SDL_RenderCopy(renderer, bloomStep1, NULL, NULL);
			SDL_RenderCopy(renderer, bloomStep2, NULL, NULL);
		}
		//Render menu screen (Disabled for now)
		//SDL_RenderCopy(renderer, titleTex, NULL, &rectTitle);

		//Draw stats text
        FC_DrawAlign(font, renderer, GAME_SCREEN_WIDTH,0,FC_ALIGN_RIGHT, "%4.2f :FPS\n%3d : MS\n%5.4lf : DT", GetFPS(), mstime, deltaTime);

		//Passes rendered image to screen
		SDL_RenderPresent(renderer);

		//Calculates the time it took to process this iteration
		mstime = SDL_GetTicks()-frameTicks;
		ProcessFPS();
		
		//Waits for at least 60 frames to be elapsed
		while( SDL_GetTicks()-frameTicks <  (1000/FRAMES_PER_SECOND) ){ }
	}
	
	//End of the program

	//Non critical dealocations
	
	free(fpscounter);
	free(keyboard_last);
	
	FreeRenderer();
	FreeModel(&Track);
	FreeCars();
	//FreeModel(&Play);
	//FreeModel(&Options);
	//FreeModel(&ExitModel);

	if(sunTex!=NULL)
	SDL_DestroyTexture(sunTex);

	if(skyTex!=NULL)
		SDL_DestroyTexture(skyTex);

	if(vigTex!=NULL)
		SDL_DestroyTexture(vigTex);

	//(Disabled for now)
	//if(titleTex!=NULL)
	//	SDL_DestroyTexture(titleTex);

	if(font!=NULL)
	FC_FreeFont(font);

	EndProgram:
	//Systems dealocation

	if(soloud!=NULL){
		Soloud_deinit(soloud);
		Soloud_destroy(soloud);
	}
			
	if(render!=NULL)
		SDL_DestroyTexture(render);

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
		Vector3 dir = scalarMult(cameraForward,50);

		MoveCamera(dir);
	}
	else if (GetKey(SDL_SCANCODE_S))
	{
		Vector3 dir = scalarMult(cameraForward,-50);
		MoveCamera(dir);
	}
	if (GetKey(SDL_SCANCODE_D))
	{
		Vector3 dir = scalarMult(cameraRight,50);
		MoveCamera(dir);
	}
	else if (GetKey(SDL_SCANCODE_A))
	{
		Vector3 dir = scalarMult(cameraRight,-50);
		MoveCamera(dir);
	}
	if (GetKey(SDL_SCANCODE_E))
	{
		Vector3 dir = scalarMult(cameraUp,10);
		MoveCamera(dir);
	}
	else if (GetKey(SDL_SCANCODE_Q))
	{
		Vector3 dir = scalarMult(cameraUp,-10);
		MoveCamera(dir);
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

	if (GetKeyDown(SDL_SCANCODE_B))
	{
		BLOOM_ENABLED = !BLOOM_ENABLED;
	}
	if(GetKey(SDL_SCANCODE_UP))
	{
		CarHandling (0, CAR_FRONT);
	}
	if(GetKey(SDL_SCANCODE_RIGHT))
	{
		CarHandling (0, CAR_RIGHT);
	}
	if(GetKey(SDL_SCANCODE_LEFT))
	{
		CarHandling (0, CAR_LEFT);
	}
	if(!GetKey(SDL_SCANCODE_UP) && !GetKey(SDL_SCANCODE_RIGHT)&& !GetKey(SDL_SCANCODE_LEFT))
	{
		CarHandling(0, CAR_STOP);
	}
	CarMovement(0);
	CarCamera(0);
}