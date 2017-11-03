#include "renderer.h"

extern const int GAME_SCREEN_WIDTH;
extern const int GAME_SCREEN_HEIGHT;
extern double deltaTime;

Pixel *screen = NULL;
SDL_Renderer* renderer = NULL;

Vector3 cameraPosition;
Vector3 cameraRotation;
Vector3 cameraForward = {0,0,-1};

void MoveCamera(Vector3 position){
    cameraPosition.x +=position.x*deltaTime;
    cameraPosition.y +=position.y*deltaTime;
    cameraPosition.z +=position.z*deltaTime;
    //printf("%f %f %f\n",cameraPosition.x,cameraPosition.y,cameraPosition.z);
}

void RotateCamera(Vector3 rotation){
    cameraRotation.x +=rotation.x*deltaTime;
    cameraRotation.y +=rotation.y*deltaTime;
    cameraRotation.z +=rotation.z*deltaTime;
    //Temporary fix for the forward vector
    cameraForward = RotatePoint((Vector3){0,0,-1},(Vector3){cameraRotation.x,360-cameraRotation.y,cameraRotation.z},(Vector3){0,0,0});
}

void TransformCamera(Vector3 position, Vector3 rotation){
    MoveCamera(position);
    RotateCamera(rotation);
}

void ClearScreen(){
    int y,x,cp = 0;
    float scanlineAdd = 0.2f*255;
    for(y=0;y<GAME_SCREEN_HEIGHT;y++){
        for(x=0;x<GAME_SCREEN_WIDTH;x++){
            if(y%10 <4){
                screen[cp].r = 0;
                screen[cp].g = 0;
                screen[cp].b = 0;
                screen[cp].a = 0;
            }else{
                screen[cp].r = scanlineAdd;
                screen[cp].g = scanlineAdd;
                screen[cp].b = scanlineAdd;
                screen[cp].a = scanlineAdd;
            }
            cp++;
        }
    }
}

Model LoadModel(char modelPath[]){
    printf("Loading ( %s )\n",modelPath);
    FILE *file = fopen(modelPath,"r");
    Model m;
    fscanf(file,"%u",&m.vCount);
    m.vertices = (Vector3*) calloc(m.vCount,sizeof(Vector3));
    printf("Vertices Count: %d\n",m.vCount);

    int i;
    for(i=0;i<m.vCount;i++){
        fscanf(file,"%f %f %f", &m.vertices[i].x, &m.vertices[i].z, &m.vertices[i].y);
    }

    fscanf(file,"%u",&m.eCount);
    m.edges = (Edge*) calloc(m.eCount,sizeof(Edge));
    printf("Edges Count: %d\n",m.eCount);

    for(i=0;i<m.eCount;i++){
        fscanf(file,"%d %d", &m.edges[i].v[0], &m.edges[i].v[1]);
    }

    m.position = (Vector3){0,0,0};
    m.rotation = (Vector3){0,0,0};
    m.enabled = 1;
    m.color = (Pixel){255,255,255,255};

    printf("> Model loaded sucessfully!\n");
    return m;
}

void FreeModel(Model *model){
    free(model->vertices);
    free(model->edges);
}

void RenderModelList(ModelList models){
    int i;

    for(i=0; i<models.oCount; i++){
        if(models.list[i].enabled){
            RenderModel(&models.list[i]);
        }
    }
}

int InitRenderer(SDL_Renderer* rend){   
    renderer = rend;
    if(renderer == NULL){
        return 0;
    }
    cameraPosition = (Vector3){0,0,0};
    cameraRotation = (Vector3){0,0,0};

    return 1;
}

void UpdateScreenPointer(Pixel* scrn){
    screen = scrn;
}

void FreeRenderer(){
 }

void RenderModel(Model *model){
    int e,v;
    float x,y,z;

    //Object Rotation
    float sinx = sin((model->rotation.x)* PI_OVER_180);
    float cosx = cos((model->rotation.x)* PI_OVER_180);

    float siny = sin((model->rotation.y) * PI_OVER_180);
    float cosy = cos((model->rotation.y) * PI_OVER_180);
    
    float sinz = sin((model->rotation.z) * PI_OVER_180);
    float cosz = cos((model->rotation.z) * PI_OVER_180);

    //Camera Rotation
    float csinx = sin((cameraRotation.x)* PI_OVER_180);
    float ccosx = cos((cameraRotation.x)* PI_OVER_180);

    float csiny = sin((cameraRotation.y) * PI_OVER_180);
    float ccosy = cos((cameraRotation.y) * PI_OVER_180);
    
    float csinz = sin((cameraRotation.z) * PI_OVER_180);
    float ccosz = cos((cameraRotation.z) * PI_OVER_180);

    //Pre calculating terms (Obj rotation)
    float rxt1 = cosy*cosz, rxt2 = (cosz*sinx*siny - cosx*sinz), rxt3 = (cosx*cosz*siny + sinx*sinz);
    float ryt1 = cosy*sinz, ryt2 = (cosx*siny*sinz - cosz*sinx), ryt3 = (cosx*cosz + sinx*siny*sinz);
    float rzt1 = cosx*cosy, rzt2 = sinx*cosy;

    //Pre calculating terms (Camera rotation)
    float crxt1 = ccosy*ccosz, crxt2 = (ccosz*csinx*csiny - ccosx*csinz), crxt3 = (ccosx*ccosz*csiny + csinx*csinz);
    float cryt1 = ccosy*csinz, cryt2 = (ccosx*csiny*csinz - ccosz*csinx), cryt3 = (ccosx*ccosz + csinx*csiny*csinz);
    float crzt1 = ccosx*ccosy, crzt2 = csinx*ccosy;

    for(e=0; e<model->eCount; e++){
        Vector3 vertices[2];
        vertices[0] = model->vertices[model->edges[e].v[0]];
        vertices[1] = model->vertices[model->edges[e].v[1]];

        //Ignore vertices that are behind the camera
        Vector3 v0 = {vertices[0].x-cameraPosition.x, vertices[0].y-cameraPosition.y, vertices[0].z-cameraPosition.z};
        Vector3 v1 = {vertices[1].x-cameraPosition.x, vertices[1].y-cameraPosition.y, vertices[1].z-cameraPosition.z};
        if(dot(v0,cameraForward)<0 || dot(v1,cameraForward)<0) continue;

        int px[2], py[2];

        for(v = 0; v <=1; v++){
            

            x = vertices[v].x;
            y = vertices[v].y;
            z = vertices[v].z;

            //Apply object rotation on the vertex
            vertices[v].x = x*rxt1 + y*rxt2 + z*rxt3;
            vertices[v].y = x*ryt1 + z*ryt2 + y*ryt3;
            vertices[v].z = z*rzt1 + y*rzt2 - x*siny;

            x = vertices[v].x - cameraPosition.x;
            y = vertices[v].y - cameraPosition.y;
            z = vertices[v].z - cameraPosition.z;

            //Apply camera rotation on the vertex
            vertices[v].x = x*crxt1 + y*crxt2 + z*crxt3;
            vertices[v].y = x*cryt1 + z*cryt2 + y*cryt3;
            vertices[v].z = z*crzt1 + y*crzt2 - x*csiny;

            //Vertex projection
            
            Vector3 e = {1,1,1000};
            px[v] = (e.z/vertices[v].z)*vertices[v].x - e.x;
            py[v] = (e.z/vertices[v].z)*vertices[v].y - e.y;

            px[v]+=GAME_SCREEN_WIDTH/2;
            py[v]+=GAME_SCREEN_HEIGHT/2;
        }
        
        //Skip the edges offscreen
        if((py[0]<0 || py[0]>GAME_SCREEN_HEIGHT) && 
           (py[1]<0 || py[1]>GAME_SCREEN_HEIGHT) && 
           (px[0]<0  || px[0]>GAME_SCREEN_WIDTH) && 
           (px[1]<0  || px[1]>GAME_SCREEN_WIDTH)) continue;

        Pixel color = model->color;

        Vector3 V0 = add(model->vertices[model->edges[e].v[0]],model->position);
        float dist = pow(V0.x-cameraPosition.x,2) + pow(V0.y-cameraPosition.y,2) + pow(V0.z-cameraPosition.z,2); 
        const int fadeDist = 750;
        dist = clamp((fadeDist-dist)/fadeDist,0,1); 

        color.r *=dist;
        color.g *=dist;
        color.b *=dist;
        color.a *=dist;

        DrawLine(px[0], py[0], px[1], py[1],color);
    }
}

void DrawLine(int x0, int y0, int x1, int y1,Pixel color) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;
    int i,maxIterations = GAME_SCREEN_WIDTH;
    for(i=0;i<maxIterations;i++){
    if(x0>=0 && x0<GAME_SCREEN_WIDTH && y0>=0 && y0<GAME_SCREEN_HEIGHT){
        screen[x0 + y0*GAME_SCREEN_WIDTH].r = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].r+color.r,0,255);
        screen[x0 + y0*GAME_SCREEN_WIDTH].g = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].g+color.g,0,255);
        screen[x0 + y0*GAME_SCREEN_WIDTH].b = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].b+color.b,0,255);
        screen[x0 + y0*GAME_SCREEN_WIDTH].a = clamp(screen[x0 + y0*GAME_SCREEN_WIDTH].a+color.a,0,255);
    }

    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
    }
}

Vector3 RotatePoint(Vector3 p, Vector3 r, Vector3 pivot){
        float rotx,roty,rotz,x,y,z;
    
        float sinx = sin(r.x* PI_OVER_180);
        float cosx = cos(r.x* PI_OVER_180);
    
        float siny = sin(r.y * PI_OVER_180);
        float cosy = cos(r.y * PI_OVER_180);
            
        float sinz = sin(r.z * PI_OVER_180);
        float cosz = cos(r.z * PI_OVER_180);
    
        x = p.x - pivot.x;
        y = p.y - pivot.y;
        z = p.z - pivot.z;
    
        rotx = x*cosy*cosz + y*(cosz*sinx*siny - cosx*sinz) + z*(cosx*cosz*siny + sinx*sinz);
        roty = x*cosy*sinz + z*(cosx*siny*sinz - cosz*sinx) + y*(cosx*cosz + sinx*siny*sinz);
        rotz = z*cosx*cosy + y*sinx*cosy - x*siny;
    
        x = rotx + pivot.x;
        y = roty + pivot.y;
        z = rotz + pivot.z;
    
        p.x = x;
        p.y = y;
        p.z = z;
        return p;
    }