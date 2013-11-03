#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
//#include <SDL2/SDL_opengles.h>
//#include <SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <assert.h>

typedef struct RenderData {
    float tick;
    float alpha;
    float setalpha;
    GLuint vertBufferId;
    GLuint texBufferId;
    GLuint texCoordLocation;
    GLuint positionLocation;
    GLuint colorLocation;
    GLuint alphaLocation;
    GLuint imgAlphaLocation;
    GLuint mvLocation;
    GLuint program;
    GLuint programSolid;
    GLfloat model[16];
    GLfloat model_view_projection[16];
    char drawState;
} RenderData;

SDL_Window* window;


#ifndef HAVE_BUILTIN_SINCOS
#define sincos _sincos
static void sincos (double a, double *s, double *c){
    *s = sin (a);
    *c = cos (a);
}
#endif


#define WIDTH 1200.0
#define HEIGHT 800.0

#define SCALE 0.011
#define SCALEXY 0.011

static void scale(GLfloat *m, float x, float y, float z) {
    m[0] *= x;
    m[1] *= x;
    m[2] *= x;
    m[3] *= x;
    m[4] *= y;
    m[5] *= y;
    m[6] *= y;
    m[7] *= y;
    m[8] *= z;
    m[9] *= z;
    m[10] *= z;
    m[11] *= z;
}


static void transpose(GLfloat *m) {
    GLfloat t[16] = {
        m[0], m[4], m[8],  m[12],
        m[1], m[5], m[9],  m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]};

    memcpy(m, t, sizeof(t));
}


static void multiply(GLfloat *m, const GLfloat *n) {
    GLfloat tmp[16];
    const GLfloat *row, *column;
    div_t d;
    int i, j;
 
    for (i = 0; i < 16; i++) {
       tmp[i] = 0;
       d = div(i, 4);
       row = n + d.quot * 4;
       column = m + d.rem;
       for (j = 0; j < 4; j++)
          tmp[i] += row[j] * column[j * 4];
    }
    memcpy(m, &tmp, sizeof tmp);
}

static void translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z) {
    GLfloat t[16] = { 1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  x, y, z, 1.0 };

    multiply(m, t);
}


static void translate2(GLfloat *a, GLfloat x, GLfloat y, GLfloat z) {
    a[12] = x;
    a[13] = y;
    a[14] = z;
    a[15] = 1.0;
}


static void identity(GLfloat *m) {
    m[0] = 1.0;
    m[1] = 0.0;
    m[2] = 0.0;
    m[3] = 0.0;
    m[4] = 0.0;
    m[5] = 1.0;
    m[6] = 0.0;
    m[7] = 0.0;
    m[8] = 0.0;
    m[9] = 0.0;
    m[10] = 1.0;
    m[11] = 0.0;
    m[12] = 0.0;
    m[13] = 0.0;
    m[14] = 0.0;
    m[15] = 1.0;
}


static void perspective(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
    GLfloat tmp[16];
    identity(tmp);
 
    double sine, cosine, cotangent, deltaZ;
    GLfloat radians = fovy / 2 * M_PI / 180;
 
    deltaZ = zFar - zNear;
    sincos(radians, &sine, &cosine);
 
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
       return;
 
    cotangent = cosine / sine;
 
    tmp[0] = cotangent / aspect;
    tmp[5] = cotangent;
    tmp[10] = -(zFar + zNear) / deltaZ;
    tmp[11] = -1;
    tmp[14] = -2 * zNear * zFar / deltaZ;
    tmp[15] = 0;
 
    memcpy(m, tmp, sizeof(tmp));
}

static void makeProgram( RenderData *renderData) {

    GLint ok;

    const char *vertexShader =  "attribute vec3 position;\n"
                                "attribute vec2 texCoord0;\n"
                                "varying vec2 v_texCoord0;\n"
                                "varying float dist;\n"
                                "uniform mat4 modelView;\n"
                                "uniform mat4 projection;\n"
                                "void main(void)\n"
                                "{\n"
                                "    vec4 p = modelView*vec4(position,1.0);\n"
                                "    gl_Position = projection*p;\n"
                                "    v_texCoord0 = texCoord0;\n"
                                "    dist = gl_Position.z*0.1;\n"
                                "}\n";


    const char *fragmentShader =
                                //"precision mediump float;\n"
                                "varying vec2 v_texCoord0;\n"
                                "varying float dist;\n"
                                "uniform sampler2D texUnit0;\n"
                                "uniform float alpha;\n"
                                "uniform float imgAlpha;\n"
                                "uniform vec4 color;\n"
                                "void main()\n"
                                "{\n"
                                "    vec4 txt = texture2D(texUnit0, v_texCoord0)*imgAlpha;\n"
                                "    gl_FragColor =vec4(1.0,1.0,1.0,1.0)*(1.0-dist);\n"
                                "    gl_FragColor.a =1.0;\n"
                                "}\n";



    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    
    glShaderSource(vs, 1, &vertexShader, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    
    assert(ok);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(fs, 1, &fragmentShader, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);


    assert(ok);

    GLuint program = glCreateProgram();
    renderData->program = program;
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    assert(ok);

    glUseProgram(program);

    identity(renderData->model);
    translate2(renderData->model,-0.2, 0.2, -50.6);

    perspective(renderData->model_view_projection, 90.0, WIDTH/HEIGHT, 1.0, 500.0);

    renderData->mvLocation = glGetUniformLocation(program, "modelView");
    glUniformMatrix4fv(renderData->mvLocation, 1, GL_FALSE, renderData->model); 

    GLint projectionLocation = glGetUniformLocation(program, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, renderData->model_view_projection); 

    GLint texLocation = glGetUniformLocation(program, "texUnit0");
    glUniform1i(texLocation, 0);

    GLint alphaLocation = glGetUniformLocation(program, "alpha");
    glUniform1f(alphaLocation, 1.0);

    GLint imgAlphaLocation = glGetUniformLocation(program, "imgAlpha");
    glUniform1f(imgAlphaLocation, 1.0);

    GLint colorLocation = glGetUniformLocation(program, "color");
    

    renderData->texCoordLocation = glGetAttribLocation(program, "texCoord0");
    renderData->positionLocation = glGetAttribLocation(program, "position");
    renderData->alphaLocation = alphaLocation;
    renderData->imgAlphaLocation = imgAlphaLocation;
    renderData->colorLocation = colorLocation;

    GLfloat vVertices[] = {1.0, -1.0, 0.0,
                            0.0, -1.0, 0.0,
                           1.0,  0.0, 0.0,
                            0.0,  0.0, 0.0};

    GLfloat vTexCoord[] = {1.0, 1.0,
                           0.0, 1.0,
                           1.0, 0.0,
                           0.0, 0.0};

    GLuint vbo;

    glEnableVertexAttribArray(renderData->positionLocation);
    glEnableVertexAttribArray(renderData->texCoordLocation);

    glGenBuffers(1, &renderData->texBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, renderData->texBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoord), vTexCoord, GL_STATIC_DRAW);
    glVertexAttribPointer(renderData->texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glGenBuffers(1, &renderData->vertBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, renderData->vertBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(renderData->positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
 
}


#define Z -6.0

void drawColorBox(struct RenderData *rd, float red,float green,float blue,float alpha,float x, float y,float z, float w, float h) {

    //printf("r%f g%f b%f a%f w%f h%f\n", red,green,blue,alpha, w, h);
    
    x -= WIDTH * 0.66;
    y -= HEIGHT * 0.66;

    identity(rd->model);
    translate2(rd->model,x*SCALEXY,-y*SCALEXY,Z+z);
    scale(rd->model, w*SCALE,h*SCALE,0);

    glUniform4f(rd->colorLocation, red,green,blue,alpha);
    
    if (rd->setalpha != 0) {
        glUniform1f(rd->imgAlphaLocation,0); 
        rd->setalpha = 0;
    } 
    glUniform1f(rd->alphaLocation, 1); 

    glUniformMatrix4fv(rd->mvLocation, 1, GL_FALSE, rd->model); 
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    

    rd->drawState = 0;


}

//**********************************************************
//**********************************************************

void draw(RenderData *rd) {

    float c = rd->tick * 0.06;
    float c2 = rd->tick * 0.023;



    int i;
    int numBoxes = 20;

    for (i=0; i<numBoxes; i++) {

        float x = 700 + sin(c+(i*0.2))*500;
        float y = i*35;
        float z = i*0.3 + sin(c2)*2 - 2;

        drawColorBox(rd, 1,0,0,1, x,y,z, 50,200);
    }

}

//**********************************************************
//**********************************************************

int loop( RenderData *rd) {

    SDL_Event e;
        
    while (SDL_PollEvent(&e)){

        if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_DOWN:   break;
                case SDLK_UP:     break;
                case SDLK_SPACE:  break;
                case SDLK_LEFT:   break;
                case SDLK_RIGHT:  break;
                default:          break;
            }
        }
        if(e.type == SDL_KEYUP){
            switch(e.key.keysym.sym) {
                case SDLK_DOWN:   break;
                case SDLK_UP:     break;
                case SDLK_SPACE:  break;
                case SDLK_LEFT:   break;
                case SDLK_RIGHT:  break;
                default:          break;
            }
        }
  
        if (e.type == SDL_QUIT) {
            return 0;
        }
    }

    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT  );
    rd->tick++;
    draw(rd);
    SDL_GL_SwapWindow(window);

    return 1;

}



void setup(RenderData *rd) {

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Renderer* displayRenderer;
    SDL_RendererInfo displayRendererInfo;

    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_OPENGL, &window, &displayRenderer);
    SDL_GetRendererInfo(displayRenderer, &displayRendererInfo);
    
    glEnable(GL_DEPTH_TEST);
    glClearColor( 0.1, 0.1, 0.1, 1.0 );
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glViewport(0, 0, WIDTH, HEIGHT);
    makeProgram(rd);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetSwapInterval(1);

}


int main(void) {
    
    
    RenderData rd;
    rd.setalpha = 1.0;
    
    setup(&rd);
    while(loop(&rd)){};

    return 0;
}