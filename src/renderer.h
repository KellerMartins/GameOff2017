#ifndef RENDERER_H
#define RENDERER_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL_FontCache.h>

#include "utils.h"

typedef struct Pixel {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
}Pixel;

typedef struct Edge {
    unsigned v[2];
}Edge;

typedef struct Model {
    int enabled;
    unsigned int timeOfActivation;

    Vector3 *vertices;
    Edge *edges;
    Pixel color;
    unsigned vCount;
    unsigned eCount;

    Vector3 position;
    Vector3 rotation;
}Model;

typedef struct ModelList {
    Model *list;
    unsigned oCount;
}ModelList;


typedef struct Ray {
    Vector3 origin;
    Vector3 direction;
    Vector3 inverseDirection;
}Ray;

// Camera rotation and vectors
extern Vector3 cameraForward;
extern Vector3 cameraUp;
extern Vector3 cameraRight;

// Fonts
extern FC_Font* fontSmall;
extern FC_Font* fontMedium;
extern FC_Font* fontBig;

void InitRenderer();
void FreeRenderer();
SDL_Renderer* GetRenderer();

Model LoadModel(char modelPath[]);
void FreeModel(Model *model);

void MoveCamera(Vector3 position);
void RotateCamera(Vector3 rotation);

void LerpCameraPosition(Vector3 pos, double t);
void LerpCameraRotation(Vector3 rot, double t);

void TransformCamera(Vector3 position, Vector3 rotation);

void BeginRender();
void EndRender();

void RenderModelList(ModelList models);
void RenderModel(Model *model);

void ToggleBloom();
bool BloomEnabled();

int GetGameWidth();
int GetGameHeight();
int GetScreenWidth();
int GetScreenHeight();
void SetResolution(int r);

void DrawLine(int x0, int y0, int x1, int y1, Pixel color);
Vector3 RotatePoint(Vector3 p, Vector3 r, Vector3 pivot);
#endif
