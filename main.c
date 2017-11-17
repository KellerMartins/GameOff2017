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
int SCREEN_WIDTH = 1366;
int SCREEN_HEIGHT = 768;

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
extern Vector3 cameraPosition;
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
void ChangeResolution(int r);
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
float skyRatio;

SDL_Texture * vigTex;
SDL_Rect rectVig;
float vigRatio;

SDL_Texture * sunTex;
SDL_Rect rectSun; 
float sunRatio;

int main(int argc, char *argv[]){

	//Initializes SDL, SDL_IMAGE, SoLoud, basic textures and general stuff
	InitProgram();
	
	//MenuState call, runs to other states until the we close the game
	if (programState != STATE_EXIT) //If initialization failed, go to free
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

	//Get user screen resolution
	SDL_DisplayMode resl;
	SDL_GetCurrentDisplayMode(0,&resl);
	SCREEN_WIDTH = resl.w;
	SCREEN_HEIGHT = resl.h;
	if(SCREEN_WIDTH == 1920) FOV = 55;
	if(SCREEN_WIDTH == 1366) FOV = 70;
	if(SCREEN_WIDTH == 1280) FOV = 80;

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
	sunRatio = sunH/(float)sunW;
	rectSun.x = GAME_SCREEN_WIDTH/2.67; rectSun.y = GAME_SCREEN_HEIGHT/4.5f; rectSun.w = GAME_SCREEN_WIDTH/4; rectSun.h = (GAME_SCREEN_WIDTH/4)*sunRatio; 
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
	skyRatio = skyH/(float)skyW;
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
	vigRatio = vigH/(float)vigW;
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

	//Game title models
	Model Play = LoadModel("Models/Play.txt");
	Play.color = (Pixel){100,30,255,255};
	Model Options = LoadModel("Models/Options.txt");
	Options.color = (Pixel){100,30,255,255};
	Model ExitModel = LoadModel("Models/Exit.txt");
	ExitModel.color = (Pixel){100,30,255,255};
	Model Arrow = LoadModel("Models/SmallArrow.txt");
	Arrow.color = (Pixel){100,30,255,255};
	Arrow.position.x = 2;
	Arrow.position.y = 2.3;

	//Game options models
	Model Resolution = LoadModel("Models/Resolution.txt");
	Resolution.color = (Pixel){100,30,255,255};
	Resolution.position.y = 2;
	Resolution.enabled = 0;

	Model R1920 = LoadModel("Models/1920x1080.txt");
	R1920.color = (Pixel){100,30,255,255};

	Model R1366 = LoadModel("Models/1366x768.txt");
	R1366.color = (Pixel){100,30,255,255};

	Model R1280 = LoadModel("Models/1280x720.txt");
	R1280.color = (Pixel){100,30,255,255};

	Model Bloom = LoadModel("Models/Bloom.txt");
	Bloom.color = (Pixel){100,30,255,255};
	Bloom.position.y = 1;
	Bloom.enabled = 0;

	Model ToggleOff = LoadModel("Models/ToggleOff.txt");
	ToggleOff.color = (Pixel){100,30,255,255};

	Model ToggleOn = LoadModel("Models/ToggleOn.txt");
	ToggleOn.color = (Pixel){100,30,255,255};

	Model BackOp = LoadModel("Models/Back.txt");
	BackOp.color = (Pixel){100,30,255,255};
	BackOp.enabled = 0;

	Model StartRace = LoadModel("Models/StartRace.txt");
	StartRace.color = (Pixel){100,30,255,255};
	StartRace.rotation.y = 15;
	StartRace.position.z = -10;
	StartRace.position.y = -1;
	StartRace.enabled = 0;

	Model BackPlay = LoadModel("Models/Back.txt");
	BackPlay.color = (Pixel){100,30,255,255};
	BackPlay.enabled = 0;
	BackPlay.position = subtract(StartRace.position,((Vector3){0,1,0}));
	BackPlay.rotation = StartRace.rotation;


	//Play screen car model
	Model Car = LoadModel("Models/Car1.txt");
	Car.color = (Pixel){100,255,30,255};
	Car.position.z = 20;

	Model Plane = LoadModel("Models/Plane.txt");
	Plane.color = (Pixel){255,60,30,100};
	Plane.position.y = -5;

	TransformCamera((Vector3){0,4.02,22.5},(Vector3){-2.16,0,0});

	//0 = Title, 1 = Play, 2 = Options
	int MenuScreen = 0;
	float cameraMovementTime = 0;
	int currentOption = 2;
	int numOptions = 3;
	//0 = 1920, 1 = 1366, 2 = 1280
	int OptResolution = 0;

	//Menu loop
	while (!Exit)
	{
		//Ms tick time
		frameTicks = SDL_GetTicks();
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST)*1000 / SDL_GetPerformanceFrequency() )*0.001;

		InputUpdate();
		
		//Input handling and option selection
		if (GetKeyDown(SDL_SCANCODE_RETURN))
		{
			
			//Pressed enter in game screen
			//Transition from game to title screen
			if(MenuScreen == 1 ){
				switch(currentOption){
					//Go to Game Race
					case 1:
						programState = STATE_GAME;
						Exit = 1;
					break;
					//Game to title
					case 0:
						cameraMovementTime = 0;
						MenuScreen = 0;
						numOptions = 3;
						currentOption = 2;

						Play.enabled = 1;
						Options.enabled = 1;
						ExitModel.enabled = 1;
						Arrow.enabled = 1;

						Play.position = (Vector3){-20,0,0};
						Options.position = (Vector3){-20,0,0};
						ExitModel.position = (Vector3){-20,0,0};
						Arrow.position = (Vector3){20,0,0};
					break;
				}

			}
			//Pressed enter in the Title screen
			else if(MenuScreen == 0){
				switch(currentOption){
					//Title to game (Option 2)
					case 2:
						cameraMovementTime = 0;
						MenuScreen = 1;
						numOptions = 2;
						currentOption = 1;

						StartRace.enabled = 1;
						BackPlay.enabled = 1;
						
						StartRace.position.x = 30;
						BackPlay.position.x = 30;
					break;
					//Title to options (Option 1)
					case 1:
						MenuScreen = 2;
						numOptions = 3;
						currentOption = 2;

						Resolution.enabled = 1;
						Bloom.enabled = 1;
						BackOp.enabled = 1;

						Resolution.position.x = 20;
						Bloom.position.x = 20;
						BackOp.position.x = 20;
					break;
					//Title to exit (Option 0)
					case 0:
						programState = STATE_EXIT;
						Exit = 1;
					break;
				}
			}
			//Pressed enter in options screen
			else if(MenuScreen == 2){
				switch(currentOption){
					//Pressed on resolution, set window resolution, recalculate values and recreate textures
					case 2:
						ChangeResolution(OptResolution);
						rectTitle.x = GAME_SCREEN_WIDTH/12;
						rectTitle.w = GAME_SCREEN_WIDTH/1.2; 
						rectTitle.h = GAME_SCREEN_WIDTH/1.2 * titleRatio;
					break;
					//Pressed on Bloom, toggle bloom
					case 1:
						BLOOM_ENABLED = !BLOOM_ENABLED;
					break;
					//Pressed on back, return to title screen
					case 0:
						MenuScreen = 0;
						numOptions = 3;
						currentOption = 2;

						Play.enabled = 1;
						Options.enabled = 1;
						ExitModel.enabled = 1;

						Arrow.position = (Vector3){20,0,0};
					break;
				}
			}
		}
		//Select up option
		if (GetKeyDown(SDL_SCANCODE_UP)|GetKeyDown(SDL_SCANCODE_W)){
			currentOption = (currentOption+1)%numOptions;
		}
		//Select down option
		if (GetKeyDown(SDL_SCANCODE_DOWN)|GetKeyDown(SDL_SCANCODE_S)){
			currentOption = (currentOption-1)%numOptions;
			currentOption = currentOption<0? numOptions + currentOption : currentOption;
		}
		//Select left option
		if (GetKeyDown(SDL_SCANCODE_LEFT)|GetKeyDown(SDL_SCANCODE_A)){
			if(MenuScreen == 2 && currentOption == 2){
				OptResolution = (OptResolution-1)%3;
				OptResolution = OptResolution<0? 3 + OptResolution : OptResolution;
			}
		}
		//Select right option
		if (GetKeyDown(SDL_SCANCODE_RIGHT)|GetKeyDown(SDL_SCANCODE_D)){
			if(MenuScreen == 2 && currentOption == 2){
				OptResolution = (OptResolution+1)%3;
			}
		}
		if (GetKey(SDL_SCANCODE_ESCAPE))
		{
			programState = STATE_EXIT;
			Exit = 1;
		}

		//Moves plane in the background
		Plane.position.z = fmod(Plane.position.z+50*deltaTime,10.0);

		//Position texts based on the current screen of the menu
		switch(MenuScreen){
			//-------------------------------------- Title Screen ---------------------------------------
			case 0:
				
				//Title go down
				if(rectTitle.y < 0){
					rectTitle.y += 1000*deltaTime;
				}else{
					rectTitle.y = 0;
				}

				//Options come from right side of the screen
				if(Play.position.x < 0){
					Play.position.x += 50*deltaTime;
				}else{
					Play.position.x = 0;
				}
				if(Options.position.x < 0){
					Options.position.x += 40*deltaTime;
				}else{
					Options.position.x = 0;
				}
				if(ExitModel.position.x < 0){
					ExitModel.position.x += 30*deltaTime;
				}else{
					ExitModel.position.x = 0;
				}
				if(Arrow.position.x > 2){
					Arrow.position.x -= 30*deltaTime;
				}else{
					Arrow.position.x = 2;
				}

				//If Options screen options are active, put them at left and disable
				if(BackOp.enabled){
					if(Resolution.position.x < 20){
						Resolution.position.x += 50*deltaTime;
					}else{
						Resolution.enabled = 0;
					}
					if(Bloom.position.x < 20){
						Bloom.position.x += 40*deltaTime;
					}else{
						Bloom.enabled = 0;
					}
					if(BackOp.position.x < 20){
						BackOp.position.x += 40*deltaTime;
					}else{
						BackOp.enabled = 0;
					}
				}
				//If Play game screen options are active, put them at left and disable
				if(StartRace.enabled){
					if(StartRace.position.x <30){
						StartRace.position.x += 60*deltaTime;
					}else{
						StartRace.enabled = 0;
					}
					if(BackPlay.position.x < 30){
						BackPlay.position.x += 50*deltaTime;
					}else{
						BackPlay.enabled = 0;
					}
				}

				switch (currentOption){
					case 0:
						Arrow.position.y = 0.35;
					break;
					case 1:
						Arrow.position.y = 1.35;
					break;
					case 2:
						Arrow.position.y = 2.35;
					break;
				}

				//Car brakes
				if(Car.position.z < 20){
					Car.position.z += 40*deltaTime;
					//Return camera to original position while the car brakes
					cameraMovementTime += deltaTime/2;

					cameraPosition.x = lerp(cameraPosition.x,0,cameraMovementTime);
					cameraPosition.y = lerp(cameraPosition.y,4.02,cameraMovementTime);
					cameraPosition.z = lerp(cameraPosition.z,22.5,cameraMovementTime);

					cameraRotation.x = lerp(cameraRotation.x,-2.16,cameraMovementTime);
					cameraRotation.y = lerp(cameraRotation.y,0,cameraMovementTime);
					cameraRotation.z = lerp(cameraRotation.z,0,cameraMovementTime);
				}else{
					Car.position.z = 20;
				}

			break;
			//-------------------------------------- Play Game ---------------------------------------
			case 1:

				//Title go up
				if(rectTitle.y > -rectTitle.h){
					rectTitle.y -= 1000*deltaTime;
				}else{
					rectTitle.y = -rectTitle.h;
				}

				//Options go towards camera
				if(Play.position.z < 20){
					Play.position.z += 50*deltaTime;
				}else{
					Play.enabled = 0;
				}
				if(Options.position.z < 20){
					Options.position.z += 50*deltaTime;
				}else{
					Options.enabled = 0;
				}
				if(ExitModel.position.z < 20){
					ExitModel.position.z += 50*deltaTime;
				}else{
					ExitModel.enabled = 0;
				}

				//Car accelerate
				if(Car.position.z > 0){
					Car.position.z -= 30*deltaTime;

					//While car hasn't stopped, move arrow out of the screen
					if(Arrow.position.z < 20){
						Arrow.position.z += 50*deltaTime;
					}
				}else{
					Car.position.z = 0;

					//Move camera closer to car after he enters the screen
					cameraMovementTime += deltaTime/2;
					cameraPosition.x = lerp(cameraPosition.x,2.961973,cameraMovementTime);
					cameraPosition.y = lerp(cameraPosition.y,1.135397,cameraMovementTime);
					cameraPosition.z = lerp(cameraPosition.z,5.717530,cameraMovementTime);

					cameraRotation.x = lerp(cameraRotation.x,-2.160000,cameraMovementTime);
					cameraRotation.y = lerp(cameraRotation.y,-15.240041,cameraMovementTime);
					cameraRotation.z = lerp(cameraRotation.z,0,cameraMovementTime);

					//Make arrow reappear and move StartRace and BackPlay to position
					Arrow.rotation = StartRace.rotation;
					if(StartRace.position.x > 2){
						StartRace.position.x -= 58*deltaTime;
					}else{
						StartRace.position.x = 2;
					}

					if(BackPlay.position.x > 2){
						BackPlay.position.x -= 58*deltaTime;
					}else{
						BackPlay.position.x = 2;
					}
				}

				switch (currentOption){
					case 1:
						Arrow.position = StartRace.position;
						Arrow.position.x += 2.1;
						Arrow.position.y +=0.35;
					break;
					case 0:
						Arrow.position = StartRace.position;
						Arrow.position.x += 2.1;
						Arrow.position.y -=0.65;
					break;
				}

			break;
			//-------------------------------------- Options ---------------------------------------
			case 2:
				
				//Title screen options goes to right
				if(Play.position.x > -20){
					Play.position.x -= 50*deltaTime;
					
					//Put arrow away, to return it later
					if(Arrow.position.x < 20){
						Arrow.position.x += 50*deltaTime;
					}else{
						Arrow.position.x = 20;
					}

				}else{
					Play.position.x = -20;

					//After this element is off screen, return the arrow to screen
					if(Arrow.position.x > 4){
						Arrow.position.x -= 50*deltaTime;
					}else{
						Arrow.position.x = 3.9;
					}
				}
				if(Options.position.x > -20){
					Options.position.x -= 40*deltaTime;
				}else{
					Options.position.x = -20;
				}
				if(ExitModel.position.x > -20){
					ExitModel.position.x -= 30*deltaTime;
				}else{
					ExitModel.position.x = -20;
				}

				//Options screen options come from the left
				if(Resolution.position.x > 0){
					Resolution.position.x -= 50*deltaTime;
				}else{
					Resolution.position.x = 0;
				}
				if(Bloom.position.x > 0){
					Bloom.position.x -= 40*deltaTime;
				}else{
					Bloom.position.x = 0;
				}
				if(BackOp.position.x > 0){
					BackOp.position.x -= 40*deltaTime;
				}else{
					BackOp.position.x = 0;
				}

				switch (currentOption){
					case 0:
						Arrow.position.y = 0.35;
					break;
					case 1:
						Arrow.position.y = 1.35;
					break;
					case 2:
						Arrow.position.y = 2.35;
					break;
				}

				//Car brakes
				if(Car.position.z < 20){
					Car.position.z += 40*deltaTime;
					//Return camera to original position while the car brakes
					cameraMovementTime += deltaTime/2;

					cameraPosition.x = lerp(cameraPosition.x,0,cameraMovementTime);
					cameraPosition.y = lerp(cameraPosition.y,4.02,cameraMovementTime);
					cameraPosition.z = lerp(cameraPosition.z,22.5,cameraMovementTime);

					cameraRotation.x = lerp(cameraRotation.x,-2.16,cameraMovementTime);
					cameraRotation.y = lerp(cameraRotation.y,0,cameraMovementTime);
					cameraRotation.z = lerp(cameraRotation.z,0,cameraMovementTime);
				}else{
					Car.position.z = 20;
				}

			break;
		}

		
		//Rendering
		//Locks texture to manual modification
		SDL_LockTexture(render, NULL, (void**)&renderPix, &renderPitch);
			UpdateScreenPointer(renderPix);
			ClearScreen();

			RenderModel(&Play);
			RenderModel(&Options);
			RenderModel(&ExitModel);
			RenderModel(&Arrow);

			RenderModel(&Resolution);
			RenderModel(&Bloom);
			RenderModel(&BackOp);

			RenderModel(&BackPlay);
			RenderModel(&StartRace);

			RenderModel(&Car);
			RenderModel(&Plane);

			//Render the options
			if(MenuScreen == 2){
				if(BLOOM_ENABLED){
					ToggleOn.position = (Vector3){Bloom.position.x-0.6,1.3,0};
					RenderModel(&ToggleOn);
				}else{
					ToggleOff.position = (Vector3){Bloom.position.x-0.6,1.3,0};
					RenderModel(&ToggleOff);
				}

				if(OptResolution == 0){
					R1920.position = (Vector3){Resolution.position.x,2,0};
					RenderModel(&R1920);
				}else if(OptResolution == 1){
					R1366.position = (Vector3){Resolution.position.x,2,0};
					RenderModel(&R1366);
				}else{
					R1280.position = (Vector3){Resolution.position.x,2,0};
					RenderModel(&R1280);
				}
			}

			if(BLOOM_ENABLED){
				//Process first bloom pass
				SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
					RenderBloom(bloomS1Pix,BLOOMS1_DOWNSCALE,0.7);
				SDL_UnlockTexture(bloomStep1);

				//Process second bloom pass
				SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
					RenderBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,1);
					BlurBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,4);
				SDL_UnlockTexture(bloomStep2);
			}
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

		//Constant of how much the sun must move based in the FOV
		Vector3 sunRot = {(150000/(FOV*FOV)),(120000/(FOV*FOV)),0};
		float sunAngle = fmodulus(cameraRotation.y,360.0);

		//Derivative of |sin(x/2)|, used to position sun based on camera angle
		float mult = sin(PI_OVER_180*sunAngle)/(4*fabs(cos(PI_OVER_180*sunAngle/2.0)));

		rectSun.x = mult*180* sunRot.x + GAME_SCREEN_WIDTH/2.67;
		rectSun.y = GAME_SCREEN_HEIGHT/4.5f - fmod(cameraRotation.x,180)*sunRot.y;
		SDL_RenderCopy(renderer, sunTex, NULL, &rectSun);

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
	FreeModel(&Car);
	FreeModel(&Plane);
	FreeModel(&Resolution);
	FreeModel(&Bloom);
	FreeModel(&ToggleOff);
	FreeModel(&ToggleOn);
	FreeModel(&R1920);
	FreeModel(&R1366);
	FreeModel(&R1280);
	FreeModel(&StartRace);

	if(titleTex!=NULL)
		SDL_DestroyTexture(titleTex);

	//Runs next state
	NextState();
}

extern Model TrackPath;
extern Model TrackModel;
void GameState(){
	LoadTrack("Tracks/TestTrack");

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
			
			RenderModel(&TrackModel);
			RenderModel(&TrackPath);

			RenderModel(&Fred1);
			RenderModel(&Fred2);
			RenderModel(&Fred3);

			RenderCars();

			if(BLOOM_ENABLED){
				//Process first bloom pass
				SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
					RenderBloom(bloomS1Pix,BLOOMS1_DOWNSCALE,0.7);
				SDL_UnlockTexture(bloomStep1);

				//Process second bloom pass
				SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
					RenderBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,1);
					BlurBloom(bloomS2Pix,BLOOMS2_DOWNSCALE,4);
				SDL_UnlockTexture(bloomStep2);
			}
		SDL_UnlockTexture(render);

		//Clears screen and blit the render texture into the screen
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
		SDL_RenderCopy(renderer, render, NULL, NULL);
		SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

		//Constant of how much the sun must move based in the FOV
		Vector3 sunRot = {(150000/(FOV*FOV)),(120000/(FOV*FOV)),0};
		float sunAngle = fmodulus(cameraRotation.y,360.0);

		//Derivative of |sin(x/2)|, used to position sun based on camera angle
		float mult = sin(PI_OVER_180*sunAngle)/(4*fabs(cos(PI_OVER_180*sunAngle/2.0)));

		rectSun.x = mult*180* sunRot.x + GAME_SCREEN_WIDTH/2.67;

		rectSun.y = GAME_SCREEN_HEIGHT/4.5f - fmod(cameraRotation.x,180)*sunRot.y;
		SDL_RenderCopy(renderer, sunTex, NULL, &rectSun);

		if(BLOOM_ENABLED){
			SDL_RenderCopy(renderer, bloomStep1, NULL, NULL);
			SDL_RenderCopy(renderer, bloomStep2, NULL, NULL);
		}

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

	FreeTrack();

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

void ChangeResolution(int r){
	//Change resolution value
	if(r == 0){
		SCREEN_WIDTH = 1920;
		SCREEN_HEIGHT = 1080;
	}else if(r == 1){
		SCREEN_WIDTH = 1366;
		SCREEN_HEIGHT = 768;
	}else if(r == 2){
		SCREEN_WIDTH = 1280;
		SCREEN_HEIGHT = 720;
	}
	//Set window resolution
	SDL_SetWindowSize(window,SCREEN_WIDTH,SCREEN_HEIGHT);

	//Reset FOV
	if(SCREEN_WIDTH == 1920) FOV = 55;
	if(SCREEN_WIDTH == 1366) FOV = 70;
	if(SCREEN_WIDTH == 1280) FOV = 80;

	//Recalculate values and recreate textures
	GAME_SCREEN_WIDTH = SCREEN_WIDTH/SCREEN_SCALE;
	GAME_SCREEN_HEIGHT = SCREEN_HEIGHT/SCREEN_SCALE;

	SDL_DestroyTexture(render);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"nearest");
	render = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
	SDL_RenderSetLogicalSize(renderer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
	SDL_SetTextureBlendMode(render,SDL_BLENDMODE_ADD);
	
	SDL_DestroyTexture(bloomStep1);
	SDL_DestroyTexture(bloomStep2);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"best");
	bloomStep1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS1_DOWNSCALE);
	bloomStep2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS2_DOWNSCALE);
	SDL_SetTextureBlendMode(bloomStep1,SDL_BLENDMODE_ADD);
	SDL_SetTextureBlendMode(bloomStep2,SDL_BLENDMODE_ADD);
	
	
	renderPitch = GAME_SCREEN_WIDTH * sizeof(unsigned int);
	bloomS1Pitch = (GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE ) * sizeof(unsigned int);
	bloomS2Pitch = (GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE ) * sizeof(unsigned int);

	FC_FreeFont(font);
	font = FC_CreateFont();  
	if(!FC_LoadFont(font, renderer, "Visitor.ttf",18, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL)){
        printf("Font: Error loading font!\n");
	}

	rectSun.x = GAME_SCREEN_WIDTH/2.67; 
	rectSun.y = GAME_SCREEN_HEIGHT/4.5f; 
	rectSun.w = GAME_SCREEN_WIDTH/4; 
	rectSun.h = (GAME_SCREEN_WIDTH/4)*sunRatio; 

	rectSky.w = GAME_SCREEN_WIDTH; 
	rectSky.h = GAME_SCREEN_WIDTH*skyRatio; 

	rectVig.w = GAME_SCREEN_WIDTH; 
	rectVig.h = GAME_SCREEN_WIDTH*vigRatio;
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

	AIMovement();
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