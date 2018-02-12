#ifndef GAMELOGIC_H
#define GAMELOGIC_H
#include "renderer.h"
#include "utils.h"



typedef struct Car{
    float speed;
    int effect;
    Model object;
    Vector3 next,closest,last;
    Vector3 objRotation;
    Vector3 position;
    Vector3 rotation;
}Car;

enum CarDirection {CAR_FRONT, CAR_STOP, CAR_UP, CAR_DOWN, CAR_RIGHT, CAR_LEFT};

void InitCars ();
void RenderCars ();
void FreeCars ();
void CarHandling(int player, int dir);
void CarMovement (int players);
void CarCamera(int player);

void AIMovement();

void PointInPath(Vector3 point, Vector3 direction, Vector3 *closest, Vector3 *next);
int GetPlayerRank(int player);
int RaceEnded();

void LoadTrack(char trackPath[]);
void FreeTrack();

#endif