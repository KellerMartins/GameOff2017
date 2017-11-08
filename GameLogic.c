#include "GameLogic.h"
#define MAX_SPEED 50
#define MAX_TURNSPEED 5
#define MAX_CARS 1
extern double deltaTime;
Car players[MAX_CARS];

extern Vector3 cameraRotation;

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

void CarCamera(int player){
    float distZ = -10, distY = 2;
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].object.rotation, VECTOR3_ZERO);
    Vector3 up = RotatePoint((Vector3){0,1,0}, players[player].object.rotation, VECTOR3_ZERO);
    Vector3 pos = add(players[player].object.position,add(scalarMult(forward,distZ),scalarMult(up,distY)));
    Vector3 rot = {360-players[player].object.rotation.x,360-players[player].object.rotation.y,360-players[player].object.rotation.z};
    TransformCamera(pos,rot);
}

void FreeCars ()
{
    int i;

    for(i=0; i<MAX_CARS; i++)
    {
        FreeModel(&players[i].object);
    }
}

void CarHandling(int player, int dir){
        
    if(dir == CAR_FRONT)
    {
        players[player].speed += players[player].speed < MAX_SPEED ? 11*deltaTime : 0; 
    }else if(dir == CAR_STOP)
    {
        players[player].speed -= players[player].speed- 20*deltaTime >= 0 ? 20*deltaTime : 0; 
    }else
    {
        players[player].speed -= players[player].speed- 10*deltaTime >= 0 ? 10*deltaTime : 0; 
    }

    float TurnSpeed = 0.25 + (players[player].speed/40 <MAX_TURNSPEED ? players[player].speed/40 : MAX_TURNSPEED);
    
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
}

void CarMovement (int player)
{
    //Calcula o vetor que aponta para a frente do carro
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].object.rotation, VECTOR3_ZERO);
    
    //Move o carro
    players[player].object.position = add(players[player].object.position, scalarMult(forward,players[player].speed * deltaTime)); 
}
