#include "GameLogic.h"
#define MAX_SPEED 20
#define MAX_TURNSPEED 2
#define MAX_CARS 1
extern double deltaTime;
Car players[MAX_CARS];

void InitCars ()
{
    int i;
    for(i=0; i<MAX_CARS; i++)
    {
        players[i].life = 100;
        players[i].speed = 0;
        players[i].flying = 0;
        players[i].effect = 0;

        if(i == 0)
        {
            //O modelo do player sera iniciado separadamente
           players[i].object = LoadModel("Models/Car1.txt");
        } else{
            //Modelo generico para todos os outros carros
           players[i].object = LoadModel("Models/Car1.txt");
        }
    }
}

void RenderCars ()
{
    int i;

    for(i=0; i<MAX_CARS; i++)
    {
        RenderModel(&players[i].object);
    }
}

void FreeCars ()
{
    int i;

    for(i=0; i<MAX_CARS; i++)
    {
        FreeModel(&players[i].object);
    }
}

void CarMovement (int player, int dir)
{
    
    if(dir == CAR_FRONT)
    {
        players[player].speed += players[player].speed < MAX_SPEED ? 11*deltaTime : 0; 
    }else if(dir == CAR_STOP){
        players[player].speed -= players[player].speed- 7*deltaTime >= 0 ? 7*deltaTime : 0; 
    }else{
        players[player].speed -= players[player].speed- 10*deltaTime >= 0 ? 10*deltaTime : 0; 
    }

    float TurnSpeed = players[player].speed/5 < MAX_TURNSPEED ? players[player].speed/5 : MAX_TURNSPEED;
    
    if(!players[player].flying)
    {
        switch(dir)
        {
            case CAR_LEFT: 
                    //Faz o movimento de curva na direcao que o carro esta indo
                    players[player].object.rotation.y -= TurnSpeed * 35 * deltaTime;
            break;

            case CAR_RIGHT: 
                    players[player].object.rotation.y += TurnSpeed * 35 * deltaTime;
            break;  

            default: 

            break;
        }
    } else {
        switch(dir)
        {

            case CAR_DOWN: 
                    players[player].object.rotation.x += TurnSpeed * 35 * deltaTime;
            break;

            case CAR_LEFT: 
                    players[player].object.rotation.y -= TurnSpeed * 35 * deltaTime;
            break; 

            case CAR_RIGHT: 
                    players[player].object.rotation.y += TurnSpeed * 35 * deltaTime;
            break;

            case CAR_UP: 
                    players[player].object.rotation.x -= TurnSpeed * 35 * deltaTime;
            break; 

            default:

            break;
        }
    }

    //Calcula o vetor que aponta para a frente do carro
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].object.rotation, VECTOR3_ZERO);
    
    //Move o carro
    players[player].object.position = add(players[player].object.position, scalarMult(forward,players[player].speed * deltaTime)); 
}
