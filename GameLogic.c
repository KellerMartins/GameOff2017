#include "GameLogic.h"
#define MAX_SPEED 50
#define MAX_TURNSPEED 5
#define MAX_CARS 6
extern double deltaTime;
Car players[MAX_CARS];

//Track variables
Model TrackPath;
Model TrackModel;
float TrackWidth;
Vector3 EndLine;
Vector3 SpawnPos[MAX_CARS];
Vector3 SpawnRot[MAX_CARS];

extern Vector3 cameraRotation;
//Debug points :P
extern Model Fred1;
extern Model Fred2;
extern Model Fred3;

void InitCars ()
{
    int i;
    for(i=0; i<MAX_CARS; i++)
    {
        players[i].speed = 0;
        players[i].effect = 0;

        players[i].position = SpawnPos[i];
        players[i].rotation = SpawnRot[i];

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

void AIMovement(){
    int i;
    for(i=1;i<MAX_CARS;i++){
        Vector3 forward = RotatePoint((Vector3){0,0,-1},players[i].rotation,VECTOR3_ZERO);
        forward.y = 0;
        NormalizeVector(&forward);

        Vector3 right = RotatePoint((Vector3){1,0,0},players[i].rotation,VECTOR3_ZERO);
        right.y = 0;
        NormalizeVector(&right);

        Vector3 cnVec = subtract(players[i].next,players[i].closest); //Vector from closest to next position in path
        cnVec.y = 0;
        NormalizeVector(&cnVec);

        float turnAngle = acos(dot(forward,cnVec))*RAD2DEG;
        if(i == 1){
            printf("%4.6f\n",turnAngle);
        }
        if(abs(turnAngle) > 0){
            float turnDir = dot(right,cnVec);

            if(turnDir < 0){
                CarHandling(i,CAR_RIGHT);
            }else{
                CarHandling(i,CAR_LEFT);
            }
        }

        CarHandling(i,CAR_FRONT);
        CarMovement(i);
    }
}

void CarHandling(int player, int dir){
        
    if(dir == CAR_FRONT)
    {
        if(player == 0){
            players[player].speed += players[player].speed < MAX_SPEED*1.1f ? 10*deltaTime : 0;
        }else{
            players[player].speed += players[player].speed < MAX_SPEED ? 11*deltaTime : 0; 
        }
    }else if(dir == CAR_STOP)
    {
        players[player].speed -= players[player].speed- 50*deltaTime >= 0 ? 20*deltaTime : 0; 
    }else
    {
        players[player].speed -= players[player].speed- 10*deltaTime >= 0 ? 10*deltaTime : 0; 
    }
    
    float TurnSpeed;
    if(player == 0){
        TurnSpeed = 0.25 + (players[player].speed/40 <MAX_TURNSPEED ? players[player].speed/40 : MAX_TURNSPEED);
    }else{
        TurnSpeed = 1.25 + (players[player].speed/20 <MAX_TURNSPEED ? players[player].speed/20 : MAX_TURNSPEED);
    }

    //Resets rotation (Last direction rotation counts)
    players[player].objRotation = VECTOR3_ZERO;
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
    }
}

void CarMovement (int player)
{
    //Calculate vector pointing forwards the car
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
    if(player == 0){
        Fred1.position = players[player].closest;
        Fred2.position = players[player].next;
        Fred3.position = players[player].last;
    }
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
        players[player].objRotation.x = 90-((1/DEG2RAD)*acos(dot(up,PLP1)));
        //printf("%f\n",90-((1/DEG2RAD)*acos(dot(up,PLP1))));

    }else{
        //If car is between the closest point and the next
        players[player].position.y = lerp(p2.y,p1.y,distPP2/distP1P2);

        //Set x axis rotation (to incline the car based on the slope)
        NormalizeVector(&P1P2);
        players[player].objRotation.x = 90-((1/DEG2RAD)*acos(dot(up,P1P2)));
        //printf("%f\n",90-((1/DEG2RAD)*acos(dot(up,P1P2))));
    }

    //Rotate car model
    players[player].object.rotation = add(players[player].rotation,players[player].objRotation);

    //Calculate vector fom the player to the closest point in the XZ plane
    Vector3 P1XZ = {p1.x,0,p1.z};
    Vector3 CarXZ = {players[player].position.x,0,players[player].position.z};
    Vector3 P1Car = subtract(CarXZ,P1XZ);

    //Puts player back on track if his distance to the closest point is greater than the track width
    float distP1Car = norm(P1Car);
    NormalizeVector(&P1Car);
    if(distP1Car>TrackWidth){
        players[player].position = subtract(players[player].position,scalarMult(P1Car,distP1Car-TrackWidth));

        //Slow down car
        if(players[player].speed>5 + 40*deltaTime){
            players[player].speed -=40*deltaTime;
        }
    }

    //"Collide" with other cars
    int i;
    for(i=0;i<MAX_CARS;i++){
        if(player == i) continue;

        Vector3 MeRivalVec = subtract(add(players[i].position,forward),players[player].position);

        if(norm(MeRivalVec) < 2){
            //Rival is in front of me
            if(dot(forward,MeRivalVec) > 0){

                //Slow down car
                if(players[player].speed>5 + 800*deltaTime){
                    players[player].speed -=800*deltaTime;
                }
            }
        }
    }
}

int GetPlayerRank(int player){
    int i,pos = 1;
    float dist2finishPlayer = dist(players[player].position,EndLine);

    for(i = 0;i<MAX_CARS;i++){
        float dist2finish = dist(players[i].position,EndLine);
        if(dist2finish < dist2finishPlayer){
            pos++;
        }
    }
    return pos;
}

void PointInPath(Vector3 point, Vector3 direction, Vector3 *closest, Vector3 *next)
{
    //If track is not loaded, just return the point
    if(TrackPath.vertices==NULL){
        *closest = point;
        *next = point;
        return;
    }

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

void LoadTrack(char trackPath[]){
    printf("Loading Track :( %s )\n",trackPath);
    
    //Load track model
    char modelPath[80];
    strcpy(modelPath, trackPath);
    strcat(modelPath, "Model.txt");

    TrackModel = LoadModel(modelPath);
    if(TrackModel.vertices == NULL){
        printf("> Failed to load track model!\n\n");
        TrackModel = (Model){0,0,0,0};
        TrackPath = (Model){0,0,0,0};
        return;
    }

    //Load track path
    char pathPath[80];
    strcpy(pathPath, trackPath);
    strcat(pathPath, "Path.txt");

    TrackPath = LoadModel(pathPath);
    if(TrackPath.vertices == NULL){
        printf("> Failed to load track path!\n\n");
        FreeModel(&TrackModel);
        TrackModel = (Model){0,0,0,0};
        TrackPath = (Model){0,0,0,0};
        return;
    }

    //Load track data
    char dataPath[80];
    strcpy(dataPath, trackPath);
    strcat(dataPath, "Data.txt");

    FILE *file = fopen(dataPath,"r");

    if(file == NULL){
        printf("> Failed to load track data!\n\n");
        FreeModel(&TrackModel);
        FreeModel(&TrackPath);
        TrackModel = (Model){0,0,0,0};
        TrackPath = (Model){0,0,0,0};
        return;
    }

    fscanf(file,"%f",&TrackWidth);
    fscanf(file,"%f%f%f",&EndLine.x,&EndLine.y,&EndLine.z);
    int i;
    for(i=0;i<6;i++){
        fscanf(file,"%f%f%f",&SpawnPos[i].x,&SpawnPos[i].y,&SpawnPos[i].z);
        fscanf(file,"%f%f%f",&SpawnRot[i].x,&SpawnRot[i].y,&SpawnRot[i].z);
    }

    fclose(file);

    printf("> Track loaded sucessfully!\n\n");
}

void FreeTrack(){
    FreeModel(&TrackModel);
    FreeModel(&TrackPath);
}
