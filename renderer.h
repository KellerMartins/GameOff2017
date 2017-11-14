#ifndef RENDERER_H
#define RENDERER_H
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "utils.h"

typedef struct Pixel{
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
	
}Pixel;

typedef struct Edge{
	unsigned v[2];	
}Edge;

typedef struct Model{
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

typedef struct ModelList{
	Model *list;
	unsigned oCount;
}ModelList;


typedef struct Ray{
	Vector3 origin;
	Vector3 direction;
	Vector3 inverseDirection;
}Ray;

Model LoadModel(char modelPath[]);
void FreeModel(Model *model);

void MoveCamera(Vector3 position);
void RotateCamera(Vector3 rotation);
void TransformCamera(Vector3 position, Vector3 rotation);

void ClearScreen();
void FillBackground();

int InitRenderer(SDL_Renderer* rend);
void UpdateScreenPointer(Pixel* scrn);
void FreeRenderer();
void RenderModelList(ModelList models);
void RenderModel(Model *model);

void RenderBloom(Pixel *bloomPix, unsigned downsample);
void BlurBloom(Pixel *bloomPix, unsigned downsample,int blurAmount);

void DrawLine(int x0, int y0, int x1, int y1,Pixel color);
Vector3 RotatePoint(Vector3 p, Vector3 r, Vector3 pivot);
#endif