#include "GameLogic.h"
#define MAX_SPEED 50
#define MAX_TURNSPEED 5
#define MAX_CARS 1
extern double deltaTime;
Car players[MAX_CARS];

extern Vector3 cameraRotation;
extern Model TrackPath;

//Debug points :P
extern Model Fred1;
extern Model Fred2;

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
           players[0].position = (Vector3){53.724,0.0,-158.5359};
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
    float distZ = -12, distY = 2.2;
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].rotation, VECTOR3_ZERO);
    Vector3 up = RotatePoint((Vector3){0,1,0}, players[player].rotation, VECTOR3_ZERO);
    Vector3 pos = add(players[player].position,add(scalarMult(forward,distZ),scalarMult(up,distY)));
    Vector3 rot = {360-players[player].rotation.x,360-players[player].rotation.y,360-players[player].rotation.z};
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
        players[player].speed -= players[player].speed- 50*deltaTime >= 0 ? 20*deltaTime : 0; 
    }else
    {
        players[player].speed -= players[player].speed- 10*deltaTime >= 0 ? 10*deltaTime : 0; 
    }

    float TurnSpeed = 0.25 + (players[player].speed/40 <MAX_TURNSPEED ? players[player].speed/40 : MAX_TURNSPEED);

    Vector3 dirAngle = ((Vector3){0,0,0});
    if(!players[player].flying)
    {
        switch(dir)
        {
            case CAR_LEFT: 
                    //Faz o movimento de curva na direcao que o carro esta indo
                    players[player].rotation.y -= TurnSpeed * 35 * deltaTime;
                    //Set turn angle
                    dirAngle = ((Vector3){0,-20*TurnSpeed,0});
            break;

            case CAR_RIGHT: 
                    players[player].rotation.y += TurnSpeed * 35 * deltaTime;
                    dirAngle = ((Vector3){0,20*TurnSpeed,0});
            break;  

            default: 

            break;
        }
    } else {
        switch(dir)
        {

            case CAR_DOWN: 
                    players[player].rotation.x += TurnSpeed * 35 * deltaTime;
            break;

            case CAR_LEFT: 
                    players[player].rotation.y -= TurnSpeed * 35 * deltaTime;
            break; 

            case CAR_RIGHT: 
                    players[player].rotation.y += TurnSpeed * 35 * deltaTime;
            break;

            case CAR_UP: 
                    players[player].rotation.x -= TurnSpeed * 35 * deltaTime;
            break; 

            default:

            break;
        }
    }
    //Rotate car model in the turn direction
    players[player].object.rotation = add(players[player].rotation,dirAngle);
}

void CarMovement (int player)
{
    //Calcula o vetor que aponta para a frente do carro
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].rotation, VECTOR3_ZERO);
    //printf("P: %f %f %f\n",players[player].position.x,players[player].position.y,players[player].position.z);
    
    //Move o carro
    players[player].position = add(players[player].position, scalarMult(forward,players[player].speed * deltaTime)); 
    players[player].object.position = players[player].position;
    PointInPath(players[player].position,forward,&Fred1.position,&Fred2.position);

}

void PointInPath(Vector3 point, Vector3 direction, Vector3 *closest, Vector3 *next)
{
    int i;
    *closest = TrackPath.vertices[0];
    *next = *closest;
    float distance = dist(*closest, point);
    for(i=1; i<TrackPath.vCount; i++)
    {
        if(distance > dist(TrackPath.vertices[i], point))
        {
            *closest = TrackPath.vertices[i];
            distance = dist(*closest,point);
        }
    }

    distance = dist(*next, *closest);
    Vector3 v2c; //vertice to closest
    for(i=1; i< TrackPath.vCount; i++)
    {
        v2c = subtract(TrackPath.vertices[i],*closest);
        if(dot(v2c,direction)>0)
        {
            if(distance > dist(TrackPath.vertices[i], *closest))
            {
                *next = TrackPath.vertices[i];
                distance = dist(*next, *closest);
            }
        }
    }
}
