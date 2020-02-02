#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <stdbool.h>
#include "renderer.h"
#include "utils.h"

#define MAX_SPEED 50
#define MAX_TURNSPEED 5
#define NUM_CARS 10

typedef struct Car{
    float speed;
    int effect;
    Model object;
    Vector3 next, closest, last;
    Vector3 objRotation;
    Vector3 position;
    Vector3 rotation;
}Car;

typedef struct Track {
    bool loaded;
    Model path;
    Model model;
    float width;
    Vector3 endLine;
    Vector3 spawnPos[NUM_CARS];
    Vector3 spawnRot[NUM_CARS];
}Track;

typedef enum dir {CAR_FRONT, CAR_STOP, CAR_UP, CAR_DOWN, CAR_RIGHT, CAR_LEFT} CarDirection;

void InitCars(Car players[NUM_CARS], Track t);
void FreeCars(Car players[NUM_CARS]);
void CarHandling(int id, Car car[NUM_CARS], CarDirection dir);
void CarMovement(int id, Car car[NUM_CARS], Track t);
void CarCamera(int id, Car car[NUM_CARS]);

void AIMovement(Car players[NUM_CARS], Track t);

void PointInPath(Track t, Vector3 point, Vector3 direction, Vector3 *closest, Vector3 *next);
int GetPlayerRank(int player, Car players[NUM_CARS], Track t);
int RaceEnded(int player, Car players[NUM_CARS], Track t);

Track LoadTrack(char trackPath[]);
void FreeTrack(Track t);

#endif