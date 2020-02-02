#include "game_state.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_FontCache.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "game_logic.h"
#include "renderer.h"
#include "input.h"

// Definition of the program state, changes in the end of each state to the one defined in 'programState'
State programState = STATE_MENU;

int Exit = 0;

// Time between frames
double deltaTime = 0;
unsigned int frameTicks;
unsigned int mstime = 0;
Uint64 NOW = 0;
Uint64 LAST = 0;

SDL_Surface * titleSurf;
SDL_Texture * titleTex;
int titleW, titleH;
float titleRatio;
SDL_Rect rectTitle;

Model Play;
Model Options;
Model ExitModel;
Model Arrow;
Model Resolution;
Model RNative;
Model R1920;
Model R1366;
Model R1280;
Model Bloom;
Model ToggleOff;
Model ToggleOn;
Model BackOp;
Model StartRace;
Model BackPlay;
Model CarModel;
Model Plane;

// Debug models :P
Model Fred1 = {0, 0, 0, 0};
Model Fred2 = {0, 0, 0, 0};
Model Fred3 = {0, 0, 0, 0};

void GameUpdate();

int MenuScreen;
float cameraMovementTime;
int currentOption, numOptions, OptResolution;
void MenuStateFinish();
void MenuStateLoop(void *arg) {
    // Ms tick time
    frameTicks = SDL_GetTicks();
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    deltaTime = (double)((NOW - LAST)*1000 / SDL_GetPerformanceFrequency() )*0.001;

    InputUpdate();

    // Input handling and option selection
    if (GetKeyDown(KEY_ENTER)) {
        // Pressed enter in game screen
        // Transition from game to title screen
        if (MenuScreen == 1) {
            switch (currentOption) {
                // Go to Game Race
                case 1:
                    programState = STATE_GAME;
                    Exit = 1;
                break;
                // Game to title
                case 0:
                    cameraMovementTime = 0;
                    MenuScreen = 0;
                    numOptions = 3;
                    currentOption = 2;

                    Play.enabled = 1;
                    Options.enabled = 1;
                    ExitModel.enabled = 1;
                    Arrow.enabled = 1;

                    Play.position = (Vector3) {-20, 0, 0};
                    Options.position = (Vector3) {-20, 0, 0};
                    ExitModel.position = (Vector3) {-20, 0, 0};
                    Arrow.position = (Vector3) {20, 0, 0};
                break;
            }
        }
        // Pressed enter in the Title screen
        else if (MenuScreen == 0) {
            switch (currentOption) {
                // Title to game (Option 2)
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
                // Title to options (Option 1)
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
                // Title to exit (Option 0)
                case 0:
                    programState = STATE_EXIT;
                    Exit = 1;
                break;
            }
        }
        // Pressed enter in options screen
        else if (MenuScreen == 2) {
            switch (currentOption) {
                // Pressed on Bloom, toggle bloom
                case 1:
                    ToggleBloom();
                break;
                // Pressed on back, return to title screen
                case 0:

                    MenuScreen = 0;
                    numOptions = 3;
                    currentOption = 2;

                    Play.enabled = 1;
                    Options.enabled = 1;
                    ExitModel.enabled = 1;

                    Arrow.position = (Vector3) {20, 0, 0};

                // Pressed on resolution, set window resolution, recalculate values and recreate textures
                case 2:
                    SetResolution(OptResolution);
                    rectTitle.x = GetGameWidth()/12;
                    rectTitle.w = GetGameWidth()/1.2;
                    rectTitle.h = GetGameWidth()/1.2 * titleRatio;
                break;
            }
        }
    }
    // Select up option
    if (GetKeyDown(KEY_UP)) {
        currentOption = (currentOption+1)%numOptions;
    }
    // Select down option
    if (GetKeyDown(KEY_DOWN)) {
        currentOption = (currentOption-1)%numOptions;
        currentOption = currentOption < 0? numOptions + currentOption : currentOption;
    }
    // Select left option
    if (GetKeyDown(KEY_LEFT)) {
        if (MenuScreen == 2 && currentOption == 2) {
            OptResolution = (OptResolution-1)%4;
            OptResolution = OptResolution < 0? 4 + OptResolution : OptResolution;
        }
    }
    // Select right option
    if (GetKeyDown(KEY_RIGHT)) {
        if (MenuScreen == 2 && currentOption == 2) {
            OptResolution = (OptResolution+1)%4;
        }
    }
    if (GetKey(KEY_BACK)) {
        programState = STATE_EXIT;
        Exit = 1;
    }

    // Moves plane in the background
    Plane.position.z = fmod(Plane.position.z+50*deltaTime, 10.0);

    // Position texts based on the current screen of the menu
    switch (MenuScreen) {
        // -------------------------------------- Title Screen ---------------------------------------
        case 0:
            // Title go down
            if (rectTitle.y < 0) {
                rectTitle.y += 1000*deltaTime;
            } else {
                rectTitle.y = 0;
            }

            // Options come from right side of the screen
            if (Play.position.x < 0) {
                Play.position.x += 50*deltaTime;
            } else {
                Play.position.x = 0;
            }
            if (Options.position.x < 0) {
                Options.position.x += 40*deltaTime;
            } else {
                Options.position.x = 0;
            }
            if (ExitModel.position.x < 0) {
                ExitModel.position.x += 30*deltaTime;
            } else {
                ExitModel.position.x = 0;
            }
            if (Arrow.position.x > 2) {
                Arrow.position.x -= 30*deltaTime;
            } else {
                Arrow.position.x = 2;
            }

            // If Options screen options are active, put them at left and disable
            if (BackOp.enabled) {
                if (Resolution.position.x < 20) {
                    Resolution.position.x += 50*deltaTime;
                } else {
                    Resolution.enabled = 0;
                }
                if (Bloom.position.x < 20) {
                    Bloom.position.x += 40*deltaTime;
                } else {
                    Bloom.enabled = 0;
                }
                if (BackOp.position.x < 20) {
                    BackOp.position.x += 40*deltaTime;
                } else {
                    BackOp.enabled = 0;
                }
            }
            // If Play game screen options are active, put them at left and disable
            if (StartRace.enabled) {
                if (StartRace.position.x <30) {
                    StartRace.position.x += 60*deltaTime;
                } else {
                    StartRace.enabled = 0;
                }
                if (BackPlay.position.x < 30) {
                    BackPlay.position.x += 50*deltaTime;
                } else {
                    BackPlay.enabled = 0;
                }
            }

            switch (currentOption) {
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

            // Car brakes
            if (CarModel.position.z < 20) {
                CarModel.position.z += 40*deltaTime;
                // Return camera to original position while the car brakes
                cameraMovementTime += deltaTime/2;

                LerpCameraPosition((Vector3) {0, 4.02, 22.5}, cameraMovementTime);
                LerpCameraRotation((Vector3) {-2.16, 0, 0}, cameraMovementTime);
            } else {
                CarModel.position.z = 20;
            }

        break;
        // -------------------------------------- Play Game ---------------------------------------
        case 1:

            // Title go up
            if (rectTitle.y > -rectTitle.h) {
                rectTitle.y -= 1000*deltaTime;
            } else {
                rectTitle.y = -rectTitle.h;
            }

            // Options go towards camera
            if (Play.position.z < 20) {
                Play.position.z += 50*deltaTime;
            } else {
                Play.enabled = 0;
            }
            if (Options.position.z < 20) {
                Options.position.z += 50*deltaTime;
            } else {
                Options.enabled = 0;
            }
            if (ExitModel.position.z < 20) {
                ExitModel.position.z += 50*deltaTime;
            } else {
                ExitModel.enabled = 0;
            }

            // Car accelerate
            if (CarModel.position.z > 0) {
                CarModel.position.z -= 30*deltaTime;

                // While car hasn't stopped, move arrow out of the screen
                if (Arrow.position.z < 20) {
                    Arrow.position.z += 50*deltaTime;
                }
            } else {
                CarModel.position.z = 0;

                // Move camera closer to car after he enters the screen
                cameraMovementTime += deltaTime/2;
                if (cameraMovementTime < 1) {
                    LerpCameraPosition((Vector3){2.961973, 1.135397, 5.717530}, cameraMovementTime);
                    LerpCameraRotation((Vector3) {-2.160000, -15.240041, 0}, cameraMovementTime);
                }

                // Make arrow reappear and move StartRace and BackPlay to position
                Arrow.rotation = StartRace.rotation;
                if (StartRace.position.x > 2) {
                    StartRace.position.x -= 58*deltaTime;
                } else {
                    StartRace.position.x = 2;
                }

                if (BackPlay.position.x > 2) {
                    BackPlay.position.x -= 58*deltaTime;
                } else {
                    BackPlay.position.x = 2;
                }
            }

            switch (currentOption) {
                case 1:
                    Arrow.position = StartRace.position;
                    Arrow.position.x += 2.1;
                    Arrow.position.y +=0.35;
                break;
                case 0:
                    Arrow.position = StartRace.position;
                    Arrow.position.x += 2.1;
                    Arrow.position.y -= 0.65;
                break;
            }

        break;
        // -------------------------------------- Options ---------------------------------------
        case 2:
            // Title screen options goes to right
            if (Play.position.x > -20) {
                Play.position.x -= 50*deltaTime;

                // Put arrow away, to return it later
                if (Arrow.position.x < 20) {
                    Arrow.position.x += 50*deltaTime;
                } else {
                    Arrow.position.x = 20;
                }

            } else {
                Play.position.x = -20;

                // After this element is off screen, return the arrow to screen
                if (Arrow.position.x > 4) {
                    Arrow.position.x -= 50*deltaTime;
                } else {
                    Arrow.position.x = 3.9;
                }
            }
            if (Options.position.x > -20) {
                Options.position.x -= 40*deltaTime;
            } else {
                Options.position.x = -20;
            }
            if (ExitModel.position.x > -20) {
                ExitModel.position.x -= 30*deltaTime;
            } else {
                ExitModel.position.x = -20;
            }

            // Options screen options come from the left
            if (Resolution.position.x > 0) {
                Resolution.position.x -= 50*deltaTime;
            } else {
                Resolution.position.x = 0;
            }
            if (Bloom.position.x > 0) {
                Bloom.position.x -= 40*deltaTime;
            } else {
                Bloom.position.x = 0;
            }
            if (BackOp.position.x > 0) {
                BackOp.position.x -= 40*deltaTime;
            } else {
                BackOp.position.x = 0;
            }

            switch (currentOption) {
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

            // Car brakes
            if (CarModel.position.z < 20) {
                CarModel.position.z += 40*deltaTime;
                // Return camera to original position while the car brakes
                cameraMovementTime += deltaTime/2;

                LerpCameraPosition((Vector3) {0, 4.02, 22.5}, cameraMovementTime);
                LerpCameraRotation((Vector3){-2.16, 0, 0}, cameraMovementTime);
            } else {
                CarModel.position.z = 20;
            }

        break;
    }

    // Rendering
    BeginRender();

        RenderModel(&Play);
        RenderModel(&Options);
        RenderModel(&ExitModel);
        RenderModel(&Arrow);

        RenderModel(&Resolution);
        RenderModel(&Bloom);
        RenderModel(&BackOp);

        RenderModel(&BackPlay);
        RenderModel(&StartRace);

        RenderModel(&CarModel);
        RenderModel(&Plane);

        // Render the options
        if (MenuScreen == 2) {
            if (BloomEnabled()) {
                ToggleOn.position = (Vector3) {Bloom.position.x-0.6, 1.3, 0};
                RenderModel(&ToggleOn);
            } else {
                ToggleOff.position = (Vector3) {Bloom.position.x-0.6, 1.3, 0};
                RenderModel(&ToggleOff);
            }

            if (OptResolution == 0) {
                RNative.position = (Vector3) {Resolution.position.x, 2, 0};
                RenderModel(&RNative);
            } else if (OptResolution == 1) {
                R1920.position = (Vector3) {Resolution.position.x, 2, 0};
                RenderModel(&R1920);
            } else if (OptResolution == 2) {
                R1366.position = (Vector3) {Resolution.position.x, 2, 0};
                RenderModel(&R1366);
            } else if (OptResolution == 3) {
                R1280.position = (Vector3) {Resolution.position.x, 2, 0};
                RenderModel(&R1280);
            }
        }


    EndRender();
    // Render menu screen
    SDL_RenderCopy(GetRenderer(), titleTex, NULL, &rectTitle);

    // Draw stats text
    FC_DrawAlign(fontSmall, GetRenderer(), GetGameWidth(), 0, FC_ALIGN_RIGHT,
                 "%4.2f :FPS\n%3d : MS\n%5.4lf : DT", GetFPS(), mstime, deltaTime);

    // Passes rendered image to screen
    SDL_RenderPresent(GetRenderer());

    // Calculates the time it took to process this iteration
    mstime = SDL_GetTicks()-frameTicks;
    ProcessFPS();

    #ifdef EMSCRIPTEN
    if (Exit) {
        emscripten_cancel_main_loop();
        NextState();
    }
    #endif
}

void MenuState() {
    // Game title Texture
    titleSurf = IMG_Load("Assets/Textures/Title.png");
    if (titleSurf == NULL) {
        printf("Error opening image!\n");
    }
    titleTex = SDL_CreateTextureFromSurface(GetRenderer(), titleSurf);
    SDL_FreeSurface(titleSurf);
    SDL_QueryTexture(titleTex, NULL, NULL, &titleW, &titleH);
    titleRatio = titleH/(float)titleW;
    rectTitle.x = GetGameWidth()/12;
    rectTitle.y = 0;
    rectTitle.w = GetGameWidth()/1.2;
    rectTitle.h = GetGameWidth()/1.2 * titleRatio;
    SDL_SetTextureBlendMode(titleTex, SDL_BLENDMODE_BLEND);

    // Game title models
    Play = LoadModel("Assets/Models/Play.txt");
    Play.color = (Pixel) {100, 30, 255, 255};
    Options = LoadModel("Assets/Models/Options.txt");
    Options.color = (Pixel) {100, 30, 255, 255};
    ExitModel = LoadModel("Assets/Models/Exit.txt");
    ExitModel.color = (Pixel) {100, 30, 255, 255};
    Arrow = LoadModel("Assets/Models/SmallArrow.txt");
    Arrow.color = (Pixel) {100, 30, 255, 255};
    Arrow.position.x = 2;
    Arrow.position.y = 2.3;

    // Game options models
    Resolution = LoadModel("Assets/Models/Resolution.txt");
    Resolution.color = (Pixel) {100, 30, 255, 255};
    Resolution.position.y = 2;
    Resolution.enabled = 0;

    RNative = LoadModel("Assets/Models/Native.txt");
    RNative.color = (Pixel) {100, 30, 255, 255};

    R1920 = LoadModel("Assets/Models/1920x1080.txt");
    R1920.color = (Pixel) {100, 30, 255, 255};

    R1366 = LoadModel("Assets/Models/1366x768.txt");
    R1366.color = (Pixel) {100, 30, 255, 255};

    R1280 = LoadModel("Assets/Models/1280x720.txt");
    R1280.color = (Pixel) {100, 30, 255, 255};

    Bloom = LoadModel("Assets/Models/Bloom.txt");
    Bloom.color = (Pixel) {100, 30, 255, 255};
    Bloom.position.y = 1;
    Bloom.enabled = 0;

    ToggleOff = LoadModel("Assets/Models/ToggleOff.txt");
    ToggleOff.color = (Pixel) {100, 30, 255, 255};

    ToggleOn = LoadModel("Assets/Models/ToggleOn.txt");
    ToggleOn.color = (Pixel) {100, 30, 255, 255};

    BackOp = LoadModel("Assets/Models/Back.txt");
    BackOp.color = (Pixel) {100, 30, 255, 255};
    BackOp.enabled = 0;

    StartRace = LoadModel("Assets/Models/StartRace.txt");
    StartRace.color = (Pixel) {100, 30, 255, 255};
    StartRace.rotation.y = 15;
    StartRace.position.z = -10;
    StartRace.position.y = -1;
    StartRace.enabled = 0;

    BackPlay = LoadModel("Assets/Models/Back.txt");
    BackPlay.color = (Pixel) {100, 30, 255, 255};
    BackPlay.enabled = 0;
    BackPlay.position = subtract(StartRace.position, ((Vector3) {0, 1, 0}));
    BackPlay.rotation = StartRace.rotation;


    // Play screen car model
    CarModel = LoadModel("Assets/Models/Car1.txt");
    CarModel.color = (Pixel) {100, 255, 30, 255};
    CarModel.position.z = 20;

    Plane = LoadModel("Assets/Models/Plane.txt");
    Plane.color = (Pixel) {255, 60, 30, 100};
    Plane.position.y = -5;

    TransformCamera((Vector3) {0, 4.02, 22.5}, (Vector3) {-2.16, 0, 0});

    // 0 = Title, 1 = Play, 2 = Options
    MenuScreen = 0;
    cameraMovementTime = 0;
    currentOption = 2;
    numOptions = 3;
    // 0 = native, 1 = 1920, 2 = 1366, 3 = 1280
    OptResolution = 0;

    // Start loop
    #ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(MenuStateLoop, NULL, -1, 1);
    #else
    while (!Exit) {
        MenuStateLoop(NULL);
    }
    MenuStateFinish();
    #endif
}

void MenuStateFinish() {
    // End of main menu

    FreeModel(&Play);
    FreeModel(&Options);
    FreeModel(&ExitModel);
    FreeModel(&CarModel);
    FreeModel(&Plane);
    FreeModel(&Resolution);
    FreeModel(&Bloom);
    FreeModel(&ToggleOff);
    FreeModel(&ToggleOn);
    FreeModel(&R1920);
    FreeModel(&R1366);
    FreeModel(&R1280);
    FreeModel(&StartRace);

    if (titleTex != NULL)
        SDL_DestroyTexture(titleTex);
}



Track track;
Car players[NUM_CARS];

double elapsedTime;
void GameStateFinish();
void GameStateLoop(void *arg) {
    // Ms tick time
    frameTicks = SDL_GetTicks();
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    deltaTime = (double)((NOW - LAST)*1000 / SDL_GetPerformanceFrequency() )*0.001;
    elapsedTime+=deltaTime;

    InputUpdate();
    if (elapsedTime >= 0) {
        GameUpdate();
    }
    if (GetKeyDown(KEY_ENTER) && RaceEnded(0, players, track)) {
        programState = STATE_MENU;
        Exit = 1;
    }

    // Locks texture to manual modification
    BeginRender();
        RenderModel(&track.model);
        // RenderModel(&track.path);

        // RenderModel(&Fred1);
        // RenderModel(&Fred2);
        // RenderModel(&Fred3);

        for (int i = 0; i < NUM_CARS; i++) {
            RenderModel(&players[i].object);
        }
    EndRender();

    if (elapsedTime >= 0) {
        int playerPos = GetPlayerRank(0, players, track);
        if (!RaceEnded(0, players, track)) {
            FC_Draw(fontMedium, GetRenderer(), 10, 0, "Time: %3.2f", elapsedTime);
            FC_Draw(fontBig, GetRenderer(), 50, FC_GetLineHeight(fontMedium)/2, "%d", playerPos);
            FC_Draw(fontSmall, GetRenderer(),
                    50 + FC_GetWidth(fontBig, "%d", playerPos)+FC_GetWidth(fontSmall, "st"),
                    FC_GetLineHeight(fontBig) - FC_GetLineHeight(fontMedium)/2,
                    "%s", playerPos == 1? "st":
                          playerPos == 2? "nd":
                          playerPos == 3? "rd":"th");
        } else {
            if (playerPos != 1)
            FC_DrawAlign(fontBig, GetRenderer(), GetScreenWidth()/2, GetScreenHeight()/3, FC_ALIGN_CENTER,
                         "%d%s Place!", playerPos, playerPos == 1? "st":
                                                   playerPos == 2? "nd":
                                                   playerPos == 3? "rd":"th");
            else
            FC_DrawAlign(fontBig, GetRenderer(), GetScreenWidth()/2, GetScreenHeight()/3, FC_ALIGN_CENTER, "YOU WON!");
        }
    } else {
        FC_DrawAlign(fontBig, GetRenderer(), GetScreenWidth()/2, GetScreenHeight()/3, FC_ALIGN_CENTER, "%d", (int)ceilf(-elapsedTime));
    }

    // Draw stats text
    FC_DrawAlign(fontSmall, GetRenderer(), GetGameWidth(), 0, FC_ALIGN_RIGHT,
                 "%4.2f :FPS\n%3d : MS\n%5.4lf : DT", GetFPS(), mstime, deltaTime);

    // Passes rendered image to screen
    SDL_RenderPresent(GetRenderer());

    // Calculates the time it took to process this iteration
    mstime = SDL_GetTicks()-frameTicks;
    ProcessFPS();

    #ifdef EMSCRIPTEN
    if (Exit) {
        emscripten_cancel_main_loop();
        NextState();
    }
    #endif
}

void GameState() {
    track = LoadTrack("Assets/Tracks/Track1");

    if (!track.loaded) {
        programState = STATE_MENU;
        GameStateFinish();
    }

    // Fred1 = LoadModel("Assets/Models/Fred.txt");
    // Fred1.color = (Pixel) {0, 0, 255, 255};
    // Fred2 = LoadModel("Assets/Models/Fred.txt");
    // Fred2.color = (Pixel) {0, 255, 0, 255};
    // Fred3 = LoadModel("Assets/Models/Fred.txt");
    // Fred3.color = (Pixel) {255, 0, 0, 255};

    InitCars(players, track);
    elapsedTime = -3;
    GameUpdate();
    // Game loop
    #ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(GameStateLoop, NULL, -1, 1);
    #else
    while (!Exit) {
        GameStateLoop(NULL);
    }
    GameStateFinish();
    #endif
}

void GameStateFinish() {
    // End of the game
    FreeTrack(track);

    FreeModel(&Fred1);
    FreeModel(&Fred2);
    FreeModel(&Fred3);

    FreeCars(players);
}

State GetState() {
    return programState;
}

void NextState() {
    Exit = 0;
    switch (programState) {
        case STATE_MENU:
            MenuState();
        break;
        case STATE_GAME:
            GameState();
        break;
        case STATE_EXIT:
            #ifdef EMSCRIPTEN
                programState = STATE_MENU;
                MenuState();
            #endif
        break;
    }
}

Vector3 pos, dir;

void GameUpdate() {
    // Camera movement
    /*if (GetKey(KEY_UP)) {
        Vector3 dir = scalarMult(cameraForward, 50*deltaTime);

        MoveCamera(dir);
    }
    else if (GetKey(KEY_DOWN)) {
        Vector3 dir = scalarMult(cameraForward, -50*deltaTime);
        MoveCamera(dir);
    }
    if (GetKey(KEY_RIGHT)) {
        Vector3 dir = scalarMult(cameraRight, 50*deltaTime);
        MoveCamera(dir);
    }
    else if (GetKey(KEY_LEFT)) {
        Vector3 dir = scalarMult(cameraRight, -50*deltaTime);
        MoveCamera(dir);
    }*/

    if (GetKey(KEY_BACK)) {
        programState = STATE_MENU;
        Exit = 1;
    }

    if (GetKeyDown(KEY_TOGGLE_BLOOM)) {
        ToggleBloom();
    }
    if (GetKey(KEY_UP)) {
        CarHandling(0, players, CAR_FRONT);
    }
    if (GetKey(KEY_RIGHT)) {
        CarHandling(0, players, CAR_RIGHT);
    }
    if (GetKey(KEY_LEFT)) {
        CarHandling(0, players, CAR_LEFT);
    }
    if (!GetKey(KEY_UP) && !GetKey(KEY_RIGHT)&& !GetKey(KEY_LEFT)) {
        CarHandling(0, players, CAR_STOP);
    }
    if (!RaceEnded(0, players, track)) {
        CarMovement(0, players, track);
        CarCamera(0, players);

        AIMovement(players, track);
    }
}