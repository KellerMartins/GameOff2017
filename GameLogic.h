#ifndef GAMELOGIC_H
#define GAMELOGIC_H
#include "renderer.h"
typedef struct Car{

int life;
float speed;
int flying;
int effect;
Model object;
}Car;

enum CarDirection {CAR_FRONT, CAR_STOP, CAR_UP, CAR_DOWN, CAR_RIGHT, CAR_LEFT};

void InitCars ();
void RenderCars ();
void FreeCars ();
void CarMovement (int players, int dir);

#endif