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
extern Model Fred3;

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
           players[0].objRotation = VECTOR3_ZERO;
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

    //Resets rotation (Last direction rotation counts)
    players[player].objRotation = VECTOR3_ZERO;
    if(!players[player].flying)
    {
        switch(dir)
        {
            case CAR_LEFT: 
                    //Turns the car in the direction it is moving
                    players[player].rotation.y -= TurnSpeed * 35 * deltaTime;
                    //Set turn angle
                    players[player].objRotation = ((Vector3){0,-20*TurnSpeed,0});
            break;

            case CAR_RIGHT: 
                    players[player].rotation.y += TurnSpeed * 35 * deltaTime;
                    players[player].objRotation = ((Vector3){0,20*TurnSpeed,0});
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
}

void CarMovement (int player)
{
    //Calcula o vetor que aponta para a frente do carro
    Vector3 forward = RotatePoint((Vector3){0,0,-1}, players[player].rotation, VECTOR3_ZERO);
    
    //Move the car
    players[player].position = add(players[player].position, scalarMult(forward,players[player].speed * deltaTime)); 
    players[player].object.position = players[player].position;

    //Gets closest and the next point in path
    Vector3 p1,p2;
    PointInPath(players[player].position,forward,&p1,&p2);

    players[player].next = p2;
    //Only changes the closest and the previous if the closest has changed
    if(dist(p1,players[player].closest)!=0){
        players[player].last = players[player].closest;
        players[player].closest = p1;
    }

    //Set debug points position
    Fred1.position = players[player].closest;
    Fred2.position = players[player].next;
    Fred3.position = players[player].last;
    
    Vector3 up = (Vector3){0,1,0};
    //Car position to be projected
    Vector3 projPos = players[player].position;
    
    Vector3 P1P2 = subtract(p2,p1);

    //If next and current aren't the same
    if(norm(P1P2)!=0)
    {
        //Calculate plane containing the p1, p2 and the up vector
        Vector3 P1P2PlaneNormal = cross(up,P1P2);
        NormalizeVector(&P1P2PlaneNormal);

        //Project the position into the plane
        Vector3 P1Point = subtract(players[player].position,p1);
        float dot_P1Point_P1P2Plane = dot(P1Point,P1P2PlaneNormal);

        projPos = subtract(players[player].position,scalarMult(P1P2PlaneNormal,dot_P1Point_P1P2Plane));
    }

    float distPP2 = dist(projPos,p2);
    float distP1P2 = dist(p1,p2);
    
    if(distPP2>distP1P2){
        //If car is between the previous point and the closest
        float distPP1 = dist(projPos,p1);
        float distPLP1 = dist(players[player].last,p1);
        players[player].position.y = lerp(p1.y,players[player].last.y,distPP1/distPLP1);

        //Set x axis rotation (to incline the car based on the slope)
        Vector3 PLP1 = subtract(p1,players[player].last);
        NormalizeVector(&PLP1);
        players[player].objRotation.x = 90-((1/PI_OVER_180)*acos(dot(up,PLP1)));
        //printf("%f\n",90-((1/PI_OVER_180)*acos(dot(up,PLP1))));

    }else{
        //If car is between the closest point and the next
        players[player].position.y = lerp(p2.y,p1.y,distPP2/distP1P2);

        //Set x axis rotation (to incline the car based on the slope)
        NormalizeVector(&P1P2);
        players[player].objRotation.x = 90-((1/PI_OVER_180)*acos(dot(up,P1P2)));
        //printf("%f\n",90-((1/PI_OVER_180)*acos(dot(up,P1P2))));
    }

    //Rotate car model
    players[player].object.rotation = add(players[player].rotation,players[player].objRotation);
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
