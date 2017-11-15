#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL_FontCache.h"

#ifndef __unix__
#include "soloud_c.h"
#endif

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
unsigned BLOOMS2_DOWNSCALE = 16;

unsigned FOV = 70;
int BLOOM_ENABLED = 1;

int Exit = 0;
int ErrorOcurred = 0;

char *fpscounter;
FC_Font* font = NULL;

#ifndef __unix__
Soloud *soloud = NULL;
#endif

//Time between frames
double deltaTime = 0;
unsigned int frameTicks;
unsigned int mstime = 0;
Uint64 NOW;
Uint64 LAST;

//Array with the keyboard state
const Uint8 *keyboard_current = NULL;
Uint8 *keyboard_last;
SDL_Event event;

//Definition of the program state, changes in the end of each state to the one defined in 'programState'
typedef enum State { STATE_EXIT, STATE_MENU, STATE_GAME}State;
State programState = STATE_MENU;

//Track path used in GameLogic to movement
Model TrackPath = {0,0,0,0};

//Camera rotation and vectors
extern Vector3 cameraRotation;
extern Vector3 cameraForward;
extern Vector3 cameraUp;
extern Vector3 cameraRight;

//Debug models :P
Model Fred1 = {0,0,0,0};
Model Fred2 = {0,0,0,0};
Model Fred3 = {0,0,0,0};

void InputUpdate();
void GameUpdate();

void MenuState();
void GameState();
void NextState();

void InitProgram();
void FreeAllocations();

SDL_Renderer * renderer = NULL;
SDL_Window* window = NULL;

//Render textures
SDL_Texture * render = NULL;
//Define the pointer to the textures pixel array
Pixel *renderPix;
int renderPitch;

SDL_Texture * bloomStep1 = NULL;
Pixel *bloomS1Pix;
int bloomS1Pitch;

SDL_Texture * bloomStep2 = NULL;
Pixel *bloomS2Pix;
int bloomS2Pitch;

//Content textures
SDL_Texture *skyTex;
SDL_Rect rectSky;

SDL_Texture * vigTex;
SDL_Rect rectVig;

SDL_Texture * sunTex;
SDL_Rect rectr; 

int main(int argc, char *argv[]){

	//Initializes SDL, SDL_IMAGE, SoLoud, basic textures and general stuff
	InitProgram();
	//MenuState call, runs to other states until the we close the game
	MenuState();
	//Deallocate objects and systems
	FreeAllocations();
	return 0;
}

void InitProgram(){
	//General initializations

	//Random from start time
	srand( (unsigned)time(NULL) );
	
	//Initializing SoLoud sound engine (Windows Only)
	#ifndef __unix__
	soloud = Soloud_create();
	if(Soloud_initEx(soloud,SOLOUD_CLIP_ROUNDOFF | SOLOUD_ENABLE_VISUALIZATION, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO)<0){
		printf("SoLoud could not initialize! \n");
		ErrorOcurred = 1;
		programState = STATE_EXIT;
		Exit = 1;
		FreeAllocations();
	}
	#endif
	//Initializing SDL
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
        printf("SDL could not initialize! SLD_Error: %s\n", SDL_GetError());
		ErrorOcurred = 1;
		programState = STATE_EXIT;
		Exit = 1;
		FreeAllocations();
	}
	//Initializing SDL_Image
	if( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) )
	{
		printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
		ErrorOcurred = 1;
		programState = STATE_EXIT;
		Exit = 1;
		FreeAllocations();
	}
	//Creating window
	window = SDL_CreateWindow( "Retro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(window == NULL){
		printf("Window could not be created! SDL_Error %s\n", SDL_GetError() );
		ErrorOcurred = 1;
		programState = STATE_EXIT;
		Exit = 1;
		FreeAllocations();
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
	
	
	renderPitch = GAME_SCREEN_WIDTH * sizeof(unsigned int);
	bloomS1Pitch = (GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE ) * sizeof(unsigned int);
	bloomS2Pitch = (GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE ) * sizeof(unsigned int);

	//Initialize frame counter string
	fpscounter = (char*)calloc(50,sizeof(char));

	//Initialize FPS counter
	InitFPS();
	NOW = SDL_GetPerformanceCounter();
    LAST = 0;
    
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
		ErrorOcurred = 1;
		programState = STATE_EXIT;
		Exit = 1;
		FreeAllocations();
	}

	//Sun Texture
	SDL_Surface * sunSurf = IMG_Load("Textures/Sun.png");
	if(sunSurf == NULL){
		printf("Error opening image!\n");
	}
	sunTex = SDL_CreateTextureFromSurface(renderer, sunSurf);
	SDL_FreeSurface(sunSurf);
	int sunW, sunH;
	SDL_QueryTexture(sunTex, NULL, NULL, &sunW, &sunH);
	float sunRatio = sunH/(float)sunW;
	rectr.x = GAME_SCREEN_WIDTH/2.67; rectr.y = GAME_SCREEN_HEIGHT/4.5f; rectr.w = GAME_SCREEN_WIDTH/4; rectr.h = (GAME_SCREEN_WIDTH/4)*sunRatio; 
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);

	//Sky texture
	SDL_Surface * skySurf = IMG_Load("Textures/Sky.png");
	if(skySurf == NULL){
		printf("Error opening image!\n");
	}
	skyTex = SDL_CreateTextureFromSurface(renderer, skySurf);
	SDL_FreeSurface(skySurf);
	int skyW, skyH;
	SDL_QueryTexture(skyTex, NULL, NULL, &skyW, &skyH);
	float skyRatio = skyH/(float)skyW;
	rectSky.x = 0; rectSky.y = 0; rectSky.w = GAME_SCREEN_WIDTH; rectSky.h = GAME_SCREEN_WIDTH*skyRatio; 

	//Vignette Texture
	SDL_Surface * vigSurf = IMG_Load("Textures/Vignette.png");
	if(vigSurf == NULL){
		printf("Error opening image!\n");
	}
	vigTex = SDL_CreateTextureFromSurface(renderer, vigSurf);
	SDL_FreeSurface(vigSurf);
	int vigW, vigH;
	SDL_QueryTexture(vigTex, NULL, NULL, &vigW, &vigH);
	float vigRatio = vigH/(float)vigW;
	rectVig.x = 0; rectVig.y = 0; rectVig.w = GAME_SCREEN_WIDTH; rectVig.h = GAME_SCREEN_WIDTH*vigRatio;
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_BLEND);
}

void MenuState(){
	//Game title Texture
	SDL_Surface * titleSurf = IMG_Load("Textures/Title.png");
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

	//Game options models
	Model Play = LoadModel("Models/Play.txt");
	Play.color = (Pixel){100,30,255,255};
	Model Options = LoadModel("Models/Options.txt");
	Options.color = (Pixel){100,30,255,255};
	Model ExitModel = LoadModel("Models/Exit.txt");
	ExitModel.color = (Pixel){100,30,255,255};

	Model Plane = LoadModel("Models/Plane.txt");
	Plane.color = (Pixel){255,60,30,100};
	Plane.position.y = -5;

	TransformCamera((Vector3){0,4.02,22.5},(Vector3){-2.16,0,0});

	//Menu loop
	while (!Exit)
	{
		//Ms tick time
		frameTicks = SDL_GetTicks();
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST)*1000 / SDL_GetPerformanceFrequency() )*0.001;

		Plane.position.z = fmod(Plane.position.z+50*deltaTime,10.0);
		InputUpdate();
		if (GetKeyDown(SDL_SCANCODE_RETURN))
		{
			programState = STATE_GAME;
			Exit = 1;
		}

		if (GetKey(SDL_SCANCODE_ESCAPE))
		{
			programState = STATE_EXIT;
			Exit = 1;
		}

		
		//Rendering
		//Locks texture to manual modification
		SDL_LockTexture(render, NULL, (void**)&renderPix, &renderPitch);
			UpdateScreenPointer(renderPix);
			ClearScreen();

			RenderModel(&Play);
			RenderModel(&Options);
			RenderModel(&ExitModel);
			RenderModel(&Plane);

			if(BLOOM_ENABLED){
				//Process first bloom pass
				SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
					RenderBloom(bloomS1Pix,BLOOMS1_DOWNSCALE);
				SDL_UnlockTexture(bloomStep1);

				//Process second bloom pass
				SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
					RenderBloom(bloomS2Pix,BLOOMS2_DOWNSCALE);
					BlurBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,4);
				SDL_UnlockTexture(bloomStep2);
			}
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

		//How much the sun must move as the camera rotates
		Vector3 sunRot = {(150000/(FOV*FOV)),(120000/(FOV*FOV)),0};
		rectr.x = cameraRotation.y*fmod(sunRot.x,180) + GAME_SCREEN_WIDTH/2.67;
		rectr.y = GAME_SCREEN_HEIGHT/4.5f - fmod(cameraRotation.x,180)*sunRot.y;
		SDL_RenderCopy(renderer, sunTex, NULL, &rectr);

		if(BLOOM_ENABLED){
			SDL_RenderCopy(renderer, bloomStep1, NULL, NULL);
			SDL_RenderCopy(renderer, bloomStep2, NULL, NULL);
		}
		//Render menu screen
		SDL_RenderCopy(renderer, titleTex, NULL, &rectTitle);

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
	
	//End of main menu

	FreeModel(&Play);
	FreeModel(&Options);
	FreeModel(&ExitModel);
	FreeModel(&Plane);

	if(titleTex!=NULL)
		SDL_DestroyTexture(titleTex);

	//Runs next state
	NextState();
}


void GameState(){
	Model Track = LoadModel("Models/TestTrack.txt");
	TrackPath = LoadModel("Models/TestTrackPath.txt");

	Fred1 = LoadModel("Models/Fred.txt");
	Fred1.color = (Pixel){0,0,255,255};
	Fred2 = LoadModel("Models/Fred.txt");
	Fred2.color = (Pixel){0,255,0,255};
	Fred3 = LoadModel("Models/Fred.txt");
	Fred3.color = (Pixel){255,0,0,255};

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
		if (GetKeyDown(SDL_SCANCODE_RETURN))
		{
			programState = STATE_MENU;
			Exit = 1;
		}
		
		//Locks texture to manual modification
		SDL_LockTexture(render, NULL, (void**)&renderPix, &renderPitch);
			UpdateScreenPointer(renderPix);
			ClearScreen();
			
			RenderModel(&Track);
			RenderModel(&TrackPath);

			RenderModel(&Fred1);
			RenderModel(&Fred2);
			RenderModel(&Fred3);

			RenderCars();

			if(BLOOM_ENABLED){
				//Process first bloom pass
				SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
					RenderBloom(bloomS1Pix,BLOOMS1_DOWNSCALE);
				SDL_UnlockTexture(bloomStep1);

				//Process second bloom pass
				SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
					RenderBloom(bloomS2Pix,BLOOMS2_DOWNSCALE);
					BlurBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,4);
				SDL_UnlockTexture(bloomStep2);
			}
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

		//How much the sun must move as the camera rotates
		Vector3 sunRot = {(150000/(FOV*FOV)),(120000/(FOV*FOV)),0};
		rectr.x = cameraRotation.y*fmod(sunRot.x,180) + GAME_SCREEN_WIDTH/2.67;
		rectr.y = GAME_SCREEN_HEIGHT/4.5f - fmod(cameraRotation.x,180)*sunRot.y;
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
	
	//End of the game

	FreeModel(&Track);
	FreeModel(&TrackPath);

	FreeModel(&Fred1);
	FreeModel(&Fred2);
	FreeModel(&Fred3);

	FreeCars();

	//Runs next state
	NextState();
}

void NextState(){
	Exit = 0;
	switch(programState){
		case STATE_EXIT:
			//Return states until gets back to main call and ends
			return;
		break;
		case STATE_MENU:
			MenuState();
		break;
		case STATE_GAME:
			GameState();
		break;
	}
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
				programState = STATE_EXIT;
                Exit = 1;
                break;
        }
    }
}

Vector3 pos,dir;

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
		programState = STATE_MENU;
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

void FreeAllocations(){
	//End of the program
	//Non critical dealocations
	free(fpscounter);
	free(keyboard_last);	
	FreeRenderer();

	if(sunTex!=NULL)
		SDL_DestroyTexture(sunTex);

	if(skyTex!=NULL)
		SDL_DestroyTexture(skyTex);

	if(vigTex!=NULL)
		SDL_DestroyTexture(vigTex);

	if(font!=NULL)
	FC_FreeFont(font);

	//Systems dealocation

	#ifndef __unix__
	if(soloud!=NULL){
		Soloud_deinit(soloud);
		Soloud_destroy(soloud);
	}
	#endif		

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
}