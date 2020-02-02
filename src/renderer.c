#include "renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_FontCache.h>

#define FRAMES_PER_SECOND 60

// Internal resolution, used in rendering. Defined by SCREEN_SCALE
int GAME_SCREEN_WIDTH = 640;
int GAME_SCREEN_HEIGHT = 360;

// Game window resolution
int SCREEN_WIDTH = 1366;
int SCREEN_HEIGHT = 768;
unsigned SCREEN_SCALE = 1;
unsigned BLOOMS1_DOWNSCALE = 2;
unsigned BLOOMS2_DOWNSCALE = 16;

unsigned FOV = 70;
int BLOOM_ENABLED = 1;

extern double deltaTime;

Vector3 cameraPosition = (Vector3) {0, 1.31, -4.8};
Vector3 cameraRotation = (Vector3) {0, 0, 0};
Vector3 cameraForward = {0, 0, -1};
Vector3 cameraUp = {0, -1, 0};
Vector3 cameraRight = {-1, 0, 0};

// ----------------------------------------------------- SDL2 CONTEXT ---------------------------------------------------- //

SDL_Renderer * renderer = NULL;
SDL_Window* window = NULL;

// Render textures
SDL_Texture * render = NULL;
Pixel *screen;
int renderPitch;

SDL_Texture * bloomStep1 = NULL;
Pixel *bloomS1Pix;
int bloomS1Pitch;

SDL_Texture * bloomStep2 = NULL;
Pixel *bloomS2Pix;
int bloomS2Pitch;


// Content textures
SDL_Texture *skyTex;
SDL_Rect rectSky;
float skyRatio;

SDL_Texture * vigTex;
SDL_Rect rectVig;
float vigRatio;

SDL_Texture * sunTex;
SDL_Rect rectSun;
float sunRatio;

// Fonts
FC_Font* fontSmall = NULL;
FC_Font* fontMedium = NULL;
FC_Font* fontBig = NULL;

void BlurBloom(Pixel *bloomPix, unsigned downsample, int blurAmount);
void RenderDownscale(Pixel *bloomPix, unsigned downsample, float multiplier);
void ClearScreen();

void InitRenderer() {
    // Get user screen resolution
    SDL_DisplayMode resl;
    SDL_GetCurrentDisplayMode(0, &resl);

    // Create window
    window = SDL_CreateWindow("Retro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              resl.w, resl.h, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error %s\n", SDL_GetError() );
        return;
    }

    // Initialize the renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures
    // Sun
    SDL_Surface * sunSurf = IMG_Load("Assets/Textures/Sun.png");
    if (sunSurf == NULL)
        printf("Error opening image!\n");

    sunTex = SDL_CreateTextureFromSurface(renderer, sunSurf);
    SDL_FreeSurface(sunSurf);
    int sunW, sunH;
    SDL_QueryTexture(sunTex, NULL, NULL, &sunW, &sunH);
    sunRatio = sunH/(float)sunW;
    SDL_SetTextureBlendMode(render, SDL_BLENDMODE_BLEND);

    // Sky
    SDL_Surface * skySurf = IMG_Load("Assets/Textures/Sky.png");
    if (skySurf == NULL)
        printf("Error opening image!\n");

    skyTex = SDL_CreateTextureFromSurface(renderer, skySurf);
    SDL_FreeSurface(skySurf);
    int skyW, skyH;
    SDL_QueryTexture(skyTex, NULL, NULL, &skyW, &skyH);
    skyRatio = skyH/(float)skyW;

    // Vignette
    SDL_Surface * vigSurf = IMG_Load("Assets/Textures/Vignette.png");
    if (vigSurf == NULL)
        printf("Error opening image!\n");

    vigTex = SDL_CreateTextureFromSurface(renderer, vigSurf);
    SDL_FreeSurface(vigSurf);
    int vigW, vigH;
    SDL_QueryTexture(vigTex, NULL, NULL, &vigW, &vigH);
    vigRatio = vigH/(float)vigW;
    SDL_SetTextureBlendMode(render, SDL_BLENDMODE_BLEND);

    // Initialize framebuffer textures, fonts and set resolution to native
    SetResolution(0);
}

void FreeRenderer() {
    if (sunTex != NULL)
        SDL_DestroyTexture(sunTex);

    if (skyTex != NULL)
        SDL_DestroyTexture(skyTex);

    if (vigTex != NULL)
        SDL_DestroyTexture(vigTex);

    if (fontSmall != NULL)
    FC_FreeFont(fontSmall);

    if (fontMedium != NULL)
    FC_FreeFont(fontMedium);

    if (fontBig != NULL)
    FC_FreeFont(fontBig);
    // Systems dealocation

    if (render != NULL)
        SDL_DestroyTexture(render);

    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);

    if (window != NULL)
        SDL_DestroyWindow(window);
}

void SetResolution(int r) {
    if (r == 0) {
        SDL_DisplayMode resl;
        SDL_GetCurrentDisplayMode(0, &resl);
        SCREEN_WIDTH = resl.w;
        SCREEN_HEIGHT = resl.h;
    } else if (r == 1) {
        SCREEN_WIDTH = 1920;
        SCREEN_HEIGHT = 1080;
    } else if (r == 2) {
        SCREEN_WIDTH = 1366;
        SCREEN_HEIGHT = 768;
    } else if (r == 3) {
        SCREEN_WIDTH = 1280;
        SCREEN_HEIGHT = 720;
    }
    // Set window resolution
    SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

    // (Re)set FOV
    if (SCREEN_WIDTH == 1920) FOV = 55;
    if (SCREEN_WIDTH == 1366) FOV = 70;
    if (SCREEN_WIDTH == 1280) FOV = 80;
    else FOV = 70 * 1366.0/SCREEN_WIDTH;

    // (Re)calculate screen size and (re)create textures
    GAME_SCREEN_WIDTH = SCREEN_WIDTH/SCREEN_SCALE;
    GAME_SCREEN_HEIGHT = SCREEN_HEIGHT/SCREEN_SCALE;

    if (render)
        SDL_DestroyTexture(render);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    render = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                               GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    SDL_RenderSetLogicalSize(renderer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    SDL_SetTextureBlendMode(render, SDL_BLENDMODE_ADD);

    // (Re)create the bloom textures
    if (bloomStep1)
        SDL_DestroyTexture(bloomStep1);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    bloomStep1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                   GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS1_DOWNSCALE);

    if (bloomStep2)
        SDL_DestroyTexture(bloomStep2);
    bloomStep2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                   GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE, GAME_SCREEN_HEIGHT/BLOOMS2_DOWNSCALE);
    SDL_SetTextureBlendMode(bloomStep1, SDL_BLENDMODE_ADD);
    SDL_SetTextureBlendMode(bloomStep2, SDL_BLENDMODE_ADD);


    renderPitch = GAME_SCREEN_WIDTH * sizeof(unsigned int);
    bloomS1Pitch = (GAME_SCREEN_WIDTH/BLOOMS1_DOWNSCALE ) * sizeof(unsigned int);
    bloomS2Pitch = (GAME_SCREEN_WIDTH/BLOOMS2_DOWNSCALE ) * sizeof(unsigned int);

    // (Re)initialize fonts
    if (fontSmall) FC_FreeFont(fontSmall);
    fontSmall = FC_CreateFont();
    if (!FC_LoadFont(fontSmall, renderer, "Assets/Visitor.ttf", 18, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL)) {
        printf("Font: Error loading font!\n");
    }

    if (fontMedium) FC_FreeFont(fontMedium);
    fontMedium = FC_CreateFont();
    if (!FC_LoadFont(fontMedium, renderer, "Assets/Visitor.ttf", 72, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL)) {
        printf("Font: Error loading font!\n");
    }

    if (fontBig) FC_FreeFont(fontBig);
    fontBig = FC_CreateFont();
    if (!FC_LoadFont(fontBig, renderer, "Assets/Visitor.ttf", 260, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL)) {
        printf("Font: Error loading font!\n");
    }

    rectSun.x = GAME_SCREEN_WIDTH/2.67;
    rectSun.y = GAME_SCREEN_HEIGHT/4.5f;
    rectSun.w = GAME_SCREEN_WIDTH/4;
    rectSun.h = (GAME_SCREEN_WIDTH/4)*sunRatio;

    rectSky.x = rectSky.y = 0;
    rectSky.w = GAME_SCREEN_WIDTH;
    rectSky.h = GAME_SCREEN_WIDTH*skyRatio;

    rectVig.x = rectVig.y = 0;
    rectVig.w = GAME_SCREEN_WIDTH;
    rectVig.h = GAME_SCREEN_WIDTH*vigRatio;
}

SDL_Renderer* GetRenderer() {
    return renderer;
}

void BeginRender() {
    SDL_LockTexture(render, NULL, (void**)&screen, &renderPitch);
    ClearScreen();
}

void EndRender() {
    if (BLOOM_ENABLED) {
        // Process first bloom pass
        SDL_LockTexture(bloomStep1, NULL, (void**)&bloomS1Pix, &bloomS1Pitch);
            RenderDownscale(bloomS1Pix, BLOOMS1_DOWNSCALE, 0.7);
        SDL_UnlockTexture(bloomStep1);

        // Process second bloom pass
        SDL_LockTexture(bloomStep2, NULL, (void**)&bloomS2Pix, &bloomS2Pitch);
            RenderDownscale(bloomS2Pix, BLOOMS2_DOWNSCALE, 1);
            BlurBloom(bloomS2Pix, BLOOMS2_DOWNSCALE, 4);
        SDL_UnlockTexture(bloomStep2);
    }
    SDL_UnlockTexture(render);

    // Clears screen and blit the render texture into the screen
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, skyTex, NULL, &rectSky);
    SDL_RenderCopy(renderer, render, NULL, NULL);
    SDL_RenderCopy(renderer, vigTex, NULL, &rectVig);

    // Constant of how much the sun must move based in the FOV
    Vector3 sunRot = {(150000/(FOV*FOV)), (120000/(FOV*FOV)), 0};
    float sunAngle = fmodulus(cameraRotation.y, 360.0);

    // Derivative of |sin(x/2)|, used to position sun based on camera angle
    float mult = sin(DEG2RAD*sunAngle)/(4*fabs(cos(DEG2RAD*sunAngle/2.0)));

    rectSun.x = mult*180* sunRot.x + GAME_SCREEN_WIDTH/2.67;
    rectSun.y = GAME_SCREEN_HEIGHT/4.5f - fmod(cameraRotation.x, 180)*sunRot.y;
    SDL_RenderCopy(renderer, sunTex, NULL, &rectSun);

    if (BLOOM_ENABLED) {
        SDL_RenderCopy(renderer, bloomStep1, NULL, NULL);
        SDL_RenderCopy(renderer, bloomStep2, NULL, NULL);
    }
}


// ----------------------------------------------------- LINE RENDERER ---------------------------------------------------- //

int GetGameWidth() {
    return GAME_SCREEN_WIDTH;
}

int GetGameHeight() {
    return GAME_SCREEN_HEIGHT;
}

int GetScreenWidth() {
    return SCREEN_WIDTH;
}

int GetScreenHeight() {
    return SCREEN_HEIGHT;
}

void MoveCamera(Vector3 position) {
    cameraPosition.x +=position.x;
    cameraPosition.y +=position.y;
    cameraPosition.z +=position.z;
}

void RotateCamera(Vector3 rotation) {
    cameraRotation.x +=rotation.x*deltaTime;
    cameraRotation.y +=rotation.y*deltaTime;
    cameraRotation.z +=rotation.z*deltaTime;
    // Temporary fix for the camera direction vectors (360-)
    cameraForward = RotatePoint(VECTOR3_FORWARD, (Vector3) {360-cameraRotation.x, 360-cameraRotation.y, 360-cameraRotation.z}, VECTOR3_ZERO);
    cameraUp = RotatePoint(VECTOR3_UP, (Vector3) {cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, VECTOR3_ZERO);
    cameraRight = RotatePoint(VECTOR3_RIGHT, (Vector3) {cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, VECTOR3_ZERO);
}


void LerpCameraPosition(Vector3 pos, double t) {
    cameraPosition.x = lerp(cameraPosition.x, pos.x, t);
    cameraPosition.y = lerp(cameraPosition.y, pos.y, t);
    cameraPosition.z = lerp(cameraPosition.z, pos.z, t);
}

void LerpCameraRotation(Vector3 rot, double t) {
    cameraRotation.x = lerp(cameraRotation.x, rot.x, t);
    cameraRotation.y = lerp(cameraRotation.y, rot.y, t);
    cameraRotation.z = lerp(cameraRotation.z, rot.z, t);
    // Temporary fix for the camera direction vectors (360-)
    cameraForward = RotatePoint(VECTOR3_FORWARD, (Vector3) {360-cameraRotation.x, 360-cameraRotation.y, 360-cameraRotation.z}, VECTOR3_ZERO);
    cameraUp = RotatePoint(VECTOR3_UP, (Vector3) {cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, VECTOR3_ZERO);
    cameraRight = RotatePoint(VECTOR3_RIGHT, (Vector3) {cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, VECTOR3_ZERO);
}

void TransformCamera(Vector3 position, Vector3 rotation) {
    cameraPosition.x = position.x;
    cameraPosition.y = position.y;
    cameraPosition.z = position.z;

    cameraRotation.x = rotation.x;
    cameraRotation.y = rotation.y;
    cameraRotation.z = rotation.z;

    cameraForward = RotatePoint(VECTOR3_FORWARD, (Vector3){360-cameraRotation.x, 360-cameraRotation.y, 360-cameraRotation.z}, (Vector3) {0, 0, 0});
    cameraUp = RotatePoint(VECTOR3_UP, (Vector3){cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, (Vector3) {0, 0, 0});
    cameraRight = RotatePoint(VECTOR3_RIGHT, (Vector3){cameraRotation.x, 360-cameraRotation.y, cameraRotation.z}, (Vector3) {0, 0, 0});
}

void ClearScreen() {
    int cp = 0;
    float scanlineAdd = 0.2f*255;
    for (int y = 0; y < GAME_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < GAME_SCREEN_WIDTH; x++) {
            // Clear screen with black and gray stripes, to look like a scanline
            if (y%10 < 4) {
                screen[cp].r = 0;
                screen[cp].g = 0;
                screen[cp].b = 0;
                screen[cp].a = 0;
            } else {
                screen[cp].r = scanlineAdd;
                screen[cp].g = scanlineAdd;
                screen[cp].b = scanlineAdd;
                screen[cp].a = scanlineAdd;
            }
            cp++;
        }
    }
}

void RenderDownscale(Pixel *bloomPix, unsigned downsample, float multiplier) {
    unsigned width = GAME_SCREEN_WIDTH/downsample, height = GAME_SCREEN_HEIGHT/downsample;
    int cp = 0;
    // Iterates for each pixel in the bloom texture
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Samples in the render texture the most bright pixel (hightest alpha value) in the area
            Pixel brightest = {0, 0, 0, 0};
            int avgBright = 0;
            for (int k = 0; k < downsample; k++) {
                for (int l = 0; l < downsample; l++) {
                    if ((i*GAME_SCREEN_WIDTH + j)*(downsample)+ l + k*GAME_SCREEN_WIDTH > GAME_SCREEN_WIDTH*GAME_SCREEN_HEIGHT)
                        continue;

                    Pixel current = screen[(i*GAME_SCREEN_WIDTH + j)*(downsample)+ l + k*GAME_SCREEN_WIDTH];
                    avgBright += current.a;

                    if (current.a > brightest.a)
                        brightest = current;
                }
            }
            // Set to zero if brightest is the scanline pixel
            if (brightest.a == 0.2f*255) {
                bloomPix[cp] = (Pixel) {0, 0, 0, 0};
            } else {
                avgBright = clamp(multiplier*avgBright/(downsample*downsample), 0, 255);
                bloomPix[cp] = (Pixel) {brightest.b, brightest.g, brightest.r, avgBright};
            }
            cp++;
        }
    }
}

void BlurBloom(Pixel *bloomPix, unsigned downsample, int blurAmount) {
    unsigned width = GAME_SCREEN_WIDTH/downsample, height = GAME_SCREEN_HEIGHT/downsample;
    int cp = 0;
    // Iterates for each pixel in the bloom texture
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Samples in the render texture the most bright pixel (hightest alpha value) in the area
            int b = 0;
            int g = 0;
            int r = 0;
            int a = 0;
            for (int  k= -blurAmount; k < blurAmount; k++) {
                for (int l = -blurAmount; l < blurAmount; l++) {
                    if ((i*width + j)+ l + k*width > width*height)
                        continue;

                    Pixel current = bloomPix[(i + k)*width + j+l];
                    // int val = (k==0? 1 : abs(k)) * (l==0? 1 : abs(l));
                    a += current.a;
                    r += current.r;
                    g += current.g;
                    b += current.b;
                }
            }

            a = clamp(a/(blurAmount*15), 0, 255);
            r = clamp(r/(blurAmount*15), 0, 255);
            g = clamp(g/(blurAmount*15), 0, 255);
            b = clamp(b/(blurAmount*15), 0, 255);
            bloomPix[cp] = (Pixel) {b, g, r, a};

            cp++;
        }
    }
}

void ToggleBloom() {
    BLOOM_ENABLED = !BLOOM_ENABLED;
}

bool BloomEnabled() {
    return BLOOM_ENABLED;
}

Model LoadModel(char modelPath[]) {
    printf("Loading ( %s )\n", modelPath);
    FILE *file = fopen(modelPath, "r");
    if (!file) {
        printf("> Failed to load model!\n\n");
        return (Model) {0, 0, NULL, NULL};
    }
    Model m;
    fscanf(file, "%u", &m.vCount);
    m.vertices = (Vector3*) calloc(m.vCount, sizeof(Vector3));
    printf("Vertices Count: %d\n", m.vCount);

    for (int i = 0; i < m.vCount; i++) {
        fscanf(file, "%f %f %f", &m.vertices[i].x, &m.vertices[i].z, &m.vertices[i].y);
    }

    fscanf(file, "%u", &m.eCount);
    m.edges = (Edge*) calloc(m.eCount, sizeof(Edge));
    printf("Edges Count: %d\n", m.eCount);

    for (int i = 0; i < m.eCount; i++) {
        fscanf(file, "%d %d", &m.edges[i].v[0], &m.edges[i].v[1]);
    }
    fclose(file);

    m.position = (Vector3) {0, 0, 0};
    m.rotation = (Vector3) {0, 0, 0};
    m.enabled = 1;
    m.color = (Pixel) {255, 255, 255, 255};

    printf("> Model loaded sucessfully!\n\n");
    return m;
}

void FreeModel(Model *model) {
    if (model->vertices) free(model->vertices);
    if (model->edges) free(model->edges);
}

void RenderModelList(ModelList models) {
    for (int i=0; i < models.oCount; i++) {
        if (models.list[i].enabled) {
            RenderModel(&models.list[i]);
        }
    }
}

void RenderModel(Model *model) {
    if (!model->enabled) return;

    float x, y, z;
    float focLen = 1000/tan((FOV*DEG2RAD)/2);
    // Object Rotation
    float sinx = sin((model->rotation.x)* DEG2RAD);
    float cosx = cos((model->rotation.x)* DEG2RAD);

    float siny = sin((model->rotation.y) * DEG2RAD);
    float cosy = cos((model->rotation.y) * DEG2RAD);

    float sinz = sin((model->rotation.z) * DEG2RAD);
    float cosz = cos((model->rotation.z) * DEG2RAD);

    // Camera Rotation
    float csinx = sin((cameraRotation.x)* DEG2RAD);
    float ccosx = cos((cameraRotation.x)* DEG2RAD);

    float csiny = sin((cameraRotation.y) * DEG2RAD);
    float ccosy = cos((cameraRotation.y) * DEG2RAD);

    float csinz = sin((cameraRotation.z) * DEG2RAD);
    float ccosz = cos((cameraRotation.z) * DEG2RAD);

    // Pre calculating terms (Obj rotation)
    float rxt1 = cosy*cosz, rxt2 = (cosz*sinx*siny - cosx*sinz), rxt3 = (cosx*cosz*siny + sinx*sinz);
    float ryt1 = cosy*sinz, ryt2 = (cosx*siny*sinz - cosz*sinx), ryt3 = (cosx*cosz + sinx*siny*sinz);
    float rzt1 = cosx*cosy, rzt2 = sinx*cosy;

    // Pre calculating terms (Camera rotation)
    float crxt1 = ccosy*ccosz, crxt2 = (ccosz*csinx*csiny - ccosx*csinz), crxt3 = (ccosx*ccosz*csiny + csinx*csinz);
    float cryt1 = ccosy*csinz, cryt2 = (ccosx*csiny*csinz - ccosz*csinx), cryt3 = (ccosx*ccosz + csinx*csiny*csinz);
    float crzt1 = ccosx*ccosy, crzt2 = csinx*ccosy;

    for (int e = 0; e < model->eCount; e++) {
        Vector3 vertices[2];
        vertices[0] = model->vertices[model->edges[e].v[0]];
        vertices[1] = model->vertices[model->edges[e].v[1]];

        int px[2], py[2];

        for (int v = 0; v <= 1; v++) {
            x = vertices[v].x;
            y = vertices[v].y;
            z = vertices[v].z;

            // Apply object rotation on the vertex
            vertices[v].x = x*rxt1 + y*rxt2 + z*rxt3;
            vertices[v].y = x*ryt1 + z*ryt2 + y*ryt3;
            vertices[v].z = z*rzt1 + y*rzt2 - x*siny;

            vertices[v].x += model->position.x;
            vertices[v].y += model->position.y;
            vertices[v].z += model->position.z;

            // Ignore vertices that are behind the camera
            Vector3 v2c = {vertices[v].x-cameraPosition.x, vertices[v].y-cameraPosition.y, vertices[v].z-cameraPosition.z};
            if (dot(v2c, cameraForward) < 0) goto NextEdge;

            // Apply camera rotation on the vertex
            x = vertices[v].x - cameraPosition.x;
            y = vertices[v].y - cameraPosition.y;
            z = vertices[v].z - cameraPosition.z;

            vertices[v].x = x*crxt1 + y*crxt2 + z*crxt3;
            vertices[v].y = x*cryt1 + z*cryt2 + y*cryt3;
            vertices[v].z = z*crzt1 + y*crzt2 - x*csiny;


            // Vertex projection
            px[v] = (focLen/vertices[v].z)*vertices[v].x;
            py[v] = (focLen/vertices[v].z)*vertices[v].y;

            px[v]+=GAME_SCREEN_WIDTH/2;
            py[v]+=GAME_SCREEN_HEIGHT/2;
        }

        // Skip the edges offscreen
        if ((py[0] < 0 || py[0] > GAME_SCREEN_HEIGHT) &&
            (py[1] < 0 || py[1] > GAME_SCREEN_HEIGHT) &&
            (px[0] < 0 || px[0] > GAME_SCREEN_WIDTH) &&
            (px[1] < 0 || px[1] > GAME_SCREEN_WIDTH)) continue;

        Pixel color = model->color;

        Vector3 V0 = add(model->vertices[model->edges[e].v[0]], model->position);
        float dist = pow(V0.x-cameraPosition.x, 2) + pow(V0.y-cameraPosition.y, 2) + pow(V0.z-cameraPosition.z, 2);
        const int fadeDist = 75000;
        dist = clamp((fadeDist-dist)/fadeDist, 0, 1);

        color.r *=dist;
        color.g *=dist;
        color.b *=dist;
        color.a *=dist;

        DrawLine(px[0], py[0], px[1], py[1], color);
        NextEdge:
        continue;
    }
}

void DrawLine(int x0, int y0, int x1, int y1, Pixel color) {
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy)/2, e2;
    int maxIterations = GAME_SCREEN_WIDTH*2;
    for (int i = 0; i < maxIterations; i++) {
        if (x0 >= 0 && x0 < GAME_SCREEN_WIDTH && y0 >= 0 && y0 < GAME_SCREEN_HEIGHT) {
            screen[x0 + y0*GAME_SCREEN_WIDTH].r = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].r+color.r, 0, 255);
            screen[x0 + y0*GAME_SCREEN_WIDTH].g = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].g+color.g, 0, 255);
            screen[x0 + y0*GAME_SCREEN_WIDTH].b = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].b+color.b, 0, 255);
            screen[x0 + y0*GAME_SCREEN_WIDTH].a = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].a+color.a, 0, 255);
        }

        if (x0 == x1 && y0 == y1)
            break;

        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

Vector3 RotatePoint(Vector3 p, Vector3 r, Vector3 pivot) {
        float rotx, roty, rotz, x, y, z;

        float sinx = sin(r.x* DEG2RAD);
        float cosx = cos(r.x* DEG2RAD);

        float siny = sin(r.y * DEG2RAD);
        float cosy = cos(r.y * DEG2RAD);

        float sinz = sin(r.z * DEG2RAD);
        float cosz = cos(r.z * DEG2RAD);

        x = p.x - pivot.x;
        y = p.y - pivot.y;
        z = p.z - pivot.z;

        rotx = x*cosy*cosz + y*(cosz*sinx*siny - cosx*sinz) + z*(cosx*cosz*siny + sinx*sinz);
        roty = x*cosy*sinz + z*(cosx*siny*sinz - cosz*sinx) + y*(cosx*cosz + sinx*siny*sinz);
        rotz = z*cosx*cosy + y*sinx*cosy - x*siny;

        x = rotx + pivot.x;
        y = roty + pivot.y;
        z = rotz + pivot.z;

        p.x = x;
        p.y = y;
        p.z = z;
        return p;
    }

