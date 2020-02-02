#include "game_logic.h"

extern double deltaTime;

Pixel CarColors[] = {
    {100, 255, 30, 255},
    {0, 50, 255, 255},
    {96, 255, 42, 255},
    {0, 255, 100, 255},
    {255, 0, 169, 255},
    {0, 55, 255, 255},
    {0, 0, 255, 255},
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {0, 0, 0, 255}};

// Debug points
extern Model Fred1;
extern Model Fred2;
extern Model Fred3;

void InitCars(Car players[NUM_CARS], Track t) {
    for (int i = 0; i < NUM_CARS; i++) {
        players[i].speed = 0;
        players[i].effect = 0;

        players[i].position = t.spawnPos[i];
        players[i].rotation = t.spawnRot[i];

        players[i].object = LoadModel("Assets/Models/Car1.txt");
        players[i].object.color = CarColors[i%(sizeof(CarColors)/sizeof(Pixel))];
    }
}

void CarCamera(int id, Car car[NUM_CARS]) {
    float distZ = -12, distY = 2.2;
    Vector3 forward = RotatePoint((Vector3) {0, 0, -1},  car[id].rotation,  VECTOR3_ZERO);
    Vector3 up = RotatePoint((Vector3) {0, 1, 0},  car[id].rotation,  VECTOR3_ZERO);
    Vector3 pos = add(car[id].position, add(scalarMult(forward, distZ), scalarMult(up, distY)));
    Vector3 rot = {360-car[id].rotation.x, 360-car[id].rotation.y, 360-car[id].rotation.z};
    TransformCamera(pos, rot);
}

void FreeCars(Car players[NUM_CARS]) {
    for (int i = 0; i < NUM_CARS; i++) {
        FreeModel(&players[i].object);
    }
}

void AIMovement(Car players[NUM_CARS], Track t) {
    for (int i=1; i < NUM_CARS; i++) {
        Vector3 ppz = {0, 0, players[i].position.z};
        Vector3 endz = {0, 0, t.endLine.z};
        float dist2finishPlayer = dist(ppz, endz);
        if (dist2finishPlayer < 1) continue;

        Vector3 forward = RotatePoint((Vector3) {0, 0, -1}, players[i].rotation, VECTOR3_ZERO);
        forward.y = 0;
        NormalizeVector(&forward);

        Vector3 right = RotatePoint((Vector3) {1, 0, 0}, players[i].rotation, VECTOR3_ZERO);
        right.y = 0;
        NormalizeVector(&right);

        Vector3 cnVec = subtract(players[i].next, players[i].closest);  // Vector from closest to next position in path
        if (cnVec.x != 0 && cnVec.z != 0) {
            cnVec.y = 0;
            NormalizeVector(&cnVec);

            float turnAngle = acos(dot(forward, cnVec)) * RAD2DEG;
            if (fabs(turnAngle) > 0) {
                float turnDir = dot(right, cnVec);

                if (turnDir < 0) {
                    CarHandling(i, players, CAR_RIGHT);
                } else {
                    CarHandling(i, players, CAR_LEFT);
                }
            }
        }
        CarHandling(i, players, CAR_FRONT);
        CarMovement(i, players, t);
    }
}

void CarHandling(int id, Car car[NUM_CARS], CarDirection dir) {
    if (dir == CAR_FRONT) {
        if (id == 0) {
            car[id].speed += car[id].speed < MAX_SPEED*1.018f ? 10*deltaTime : 0;
        } else {
            car[id].speed += car[id].speed < MAX_SPEED ? 20*deltaTime : 0;
        }
    } else if (dir == CAR_STOP) {
        car[id].speed -= car[id].speed- 50*deltaTime >= 0 ? 20*deltaTime : 0;
    } else {
        car[id].speed -= car[id].speed- 10*deltaTime >= 0 ? 10*deltaTime : 0;
    }

    float TurnSpeed;
    if (id == 0) {
        TurnSpeed = 0.25 + (car[id].speed/40 <MAX_TURNSPEED ? car[id].speed/40 : MAX_TURNSPEED);
    } else {
        TurnSpeed = MAX_TURNSPEED;
    }

    // Resets rotation (Last direction rotation counts)
    car[id].objRotation = VECTOR3_ZERO;
    switch (dir) {
        case CAR_LEFT:
                // Turns the car in the direction it is moving
                car[id].rotation.y -= TurnSpeed * 35 * deltaTime;
                // Set turn angle
                car[id].objRotation = ((Vector3) {0, -20*TurnSpeed, 0});
        break;

        case CAR_RIGHT:
                car[id].rotation.y += TurnSpeed * 35 * deltaTime;
                car[id].objRotation = ((Vector3) {0, 20*TurnSpeed, 0});
        break;
        default:
        break;
    }
}

void CarMovement(int id, Car car[NUM_CARS], Track t) {
    // Calculate vector pointing forwards the car
    Vector3 forward = RotatePoint((Vector3) {0, 0, -1}, car[id].rotation, VECTOR3_ZERO);

    // Move the car
    car[id].position = add(car[id].position, scalarMult(forward, car[id].speed * deltaTime));
    car[id].object.position = car[id].position;

    // Gets closest and the next point in path
    Vector3 p1, p2;
    PointInPath(t, car[id].position, forward, &p1, &p2);

    car[id].next = p2;
    // Only changes the closest and the previous if the closest has changed
    if (dist(p1, car[id].closest) != 0) {
        car[id].last = car[id].closest;
        car[id].closest = p1;
    }

    // Set debug points position
    if (id == 0) {
        Fred1.position = car[id].closest;
        Fred2.position = car[id].next;
        Fred3.position = car[id].last;
    }

    Vector3 up = (Vector3) {0, 1, 0};
    // Car position to be projected
    Vector3 projPos = car[id].position;

    Vector3 P1P2 = subtract(p2, p1);

    // If next and current aren't the same
    if (norm(P1P2) != 0) {
        // Calculate plane containing the p1, p2 and the up vector
        Vector3 P1P2PlaneNormal = cross(up, P1P2);
        NormalizeVector(&P1P2PlaneNormal);

        // Project the position into the plane
        Vector3 P1Point = subtract(car[id].position, p1);
        float dot_P1Point_P1P2Plane = dot(P1Point, P1P2PlaneNormal);

        projPos = subtract(car[id].position, scalarMult(P1P2PlaneNormal, dot_P1Point_P1P2Plane));
    }

    float distPP2 = dist(projPos, p2);
    float distP1P2 = dist(p1, p2);

    if (distPP2 > distP1P2) {
        // If car is between the previous point and the closest
        float distPP1 = dist(projPos, p1);
        float distPLP1 = dist(car[id].last, p1);
        car[id].position.y = lerp(p1.y, car[id].last.y, distPP1/distPLP1);

        // Set x axis rotation (to incline the car based on the slope)
        Vector3 PLP1 = subtract(p1, car[id].last);
        NormalizeVector(&PLP1);
        car[id].objRotation.x = 90-((1/DEG2RAD)*acos(dot(up, PLP1)));
        // printf("%f\n", 90-((1/DEG2RAD)*acos(dot(up, PLP1))));

    } else {
        // If car is between the closest point and the next
        car[id].position.y = lerp(p2.y, p1.y, distPP2/distP1P2);

        // Set x axis rotation (to incline the car based on the slope)
        NormalizeVector(&P1P2);
        car[id].objRotation.x = 90-((1/DEG2RAD)*acos(dot(up, P1P2)));
        // printf("%f\n", 90-((1/DEG2RAD)*acos(dot(up, P1P2))));
    }

    // Rotate car model
    car[id].object.rotation = add(car[id].rotation, car[id].objRotation);

    // Calculate vector fom the player to the closest point in the XZ plane
    Vector3 P1XZ = {p1.x, 0, p1.z};
    Vector3 CarXZ = {car[id].position.x, 0, car[id].position.z};
    Vector3 P1Car = subtract(CarXZ, P1XZ);

    // Puts player back on track if his distance to the closest point is greater than the track width
    float distP1Car = norm(P1Car);
    NormalizeVector(&P1Car);
    if (distP1Car > t.width) {
        car[id].position = subtract(car[id].position, scalarMult(P1Car, distP1Car-t.width));

        // Slow down car
        if (car[id].speed > 5 + 40*deltaTime) {
            car[id].speed -= 40*deltaTime;
        }
    }

    // "Collide" with other cars
    for (int i = 0; i < NUM_CARS; i++) {
        if (id == i) continue;

        Vector3 MeRivalVec = subtract(add(car[i].position, forward), car[id].position);

        if (norm(MeRivalVec) < 2) {
            // Rival is in front of me
            if (dot(forward, MeRivalVec) > 0) {
                // Slow down car
                if (car[id].speed > 5 + 800*deltaTime) {
                    car[id].speed -= 800*deltaTime;
                }
            }
        }
    }
}

int GetPlayerRank(int player, Car players[NUM_CARS], Track t) {
    int pos = 1;
    Vector3 ppz = {0, 0, players[player].position.z};
    Vector3 endz = {0, 0, t.endLine.z};
    float dist2finishPlayer = dist(ppz, endz);

    for (int i = 0; i < NUM_CARS; i++) {
        Vector3 pz = {0, 0, players[i].position.z};
        float dist2finish = dist(pz, endz);
        if (dist2finish < dist2finishPlayer) {
            pos++;
        }
    }
    return pos;
}

int RaceEnded(int player, Car players[NUM_CARS], Track t) {
    Vector3 ppz = {0, 0, players[player].position.z};
    Vector3 endz = {0, 0, t.endLine.z};
    float dist2finishPlayer = dist(ppz, endz);

    return dist2finishPlayer < 1.5;
}

void PointInPath(Track t, Vector3 point, Vector3 direction, Vector3 *closest, Vector3 *next) {
    // If track is not loaded, just return the point
    if (t.path.vertices == NULL) {
        *closest = point;
        *next = point;
        return;
    }

    *closest = t.path.vertices[0];
    *next = t.path.vertices[1];
    float distance = INFINITY;
    for (int i = 0; i < t.path.vCount; i++) {
        if (distance > dist(t.path.vertices[i], point)) {
            *closest = t.path.vertices[i];
            distance = dist(*closest, point);
        }
    }

    distance = INFINITY;
    Vector3 v2c;  // vertice to closest
    for (int i = 0; i < t.path.vCount; i++) {
        v2c = subtract(t.path.vertices[i], *closest);
        if (dot(v2c, direction) > 0) {
            if (distance > dist(t.path.vertices[i], *closest)) {
                *next = t.path.vertices[i];
                distance = dist(*next, *closest);
            }
        }
    }
}

Track LoadTrack(char trackPath[]) {
    Track t;
    printf("Loading Track :( %s )\n", trackPath);

    // Load track model
    char modelPath[80];
    strncpy(modelPath, trackPath, sizeof(modelPath)-1);
    strncat(modelPath, "Model.txt", sizeof(modelPath)-1);

    t.model = LoadModel(modelPath);
    if (t.model.vertices == NULL) {
        printf("> Failed to load track model!\n\n");
        t.model = (Model) {0, 0, 0, 0};
        t.path = (Model) {0, 0, 0, 0};
        return (Track){.loaded = false};
    }

    // Load track path
    char pathPath[80];
    strncpy(pathPath, trackPath, sizeof(pathPath)-1);
    strncat(pathPath, "Path.txt", sizeof(pathPath)-1);

    t.path = LoadModel(pathPath);
    if (t.path.vertices == NULL) {
        printf("> Failed to load track path!\n\n");
        FreeModel(&t.model);
        t.model = (Model) {0, 0, 0, 0};
        t.path = (Model) {0, 0, 0, 0};
        return (Track){.loaded = false};
    }

    // Load track data
    char dataPath[80];
    strncpy(dataPath, trackPath, sizeof(dataPath)-1);
    strncat(dataPath, "Data.txt", sizeof(dataPath)-1);

    FILE *file = fopen(dataPath, "r");

    if (file == NULL) {
        printf("> Failed to load track data!\n\n");
        FreeModel(&t.model);
        FreeModel(&t.path);
        t.model = (Model) {0, 0, 0, 0};
        t.path = (Model) {0, 0, 0, 0};
        return (Track){.loaded = false};
    }

    fscanf(file, "%f", &t.width);
    fscanf(file, "%f%f%f", &t.endLine.x, &t.endLine.z, &t.endLine.y);
    for (int i = 0; i < NUM_CARS; i++) {
        fscanf(file, "%f%f%f", &t.spawnPos[i].x, &t.spawnPos[i].z, &t.spawnPos[i].y);
        fscanf(file, "%f%f%f", &t.spawnRot[i].x, &t.spawnRot[i].z, &t.spawnRot[i].y);
    }

    fclose(file);
    printf("> Track loaded sucessfully!\n\n");

    t.loaded = true;
    return t;
}

void FreeTrack(Track t) {
    FreeModel(&t.model);
    FreeModel(&t.path);
}
