/*
 * Simple 3D sine wave animation example.
 * $Id: sinewave3D.c,v 1.7 2018/07/17 00:27:47 gl Exp gl $
 */

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>

#define VBO 0
#define IBO 1
#define NUM_BUFFERS 2
GLuint buffers[NUM_BUFFERS];

typedef enum {
  d_drawSineWave,
  d_mouse,
  d_key,
  d_animation,
  d_lighting,
  d_OSD,
  d_nflags
} DebugFlags;

bool debug[d_nflags] = { false, false, false, false, false, false };

typedef struct { float x, y; } vec2f;
typedef struct { float x, y, z; } vec3f;
typedef struct { float r, g, b; } color3f;
typedef struct { vec3f r, n; } Vertex;

typedef struct { float A; float kx; float kz; float w; } sinewave;
sinewave sws[] =
  {
   { 0.125, 2.0 * M_PI / 1.0, 0.0, 0.25 * M_PI },
   { 0.125, 0.0, 2.0 * M_PI / 1.0, 0.25 * M_PI },
#if 0
   // Extra waves which can be added
   { 0.125, 0.0, 2.0 * M_PI / 1.0, 0.25 * M_PI },
   { 0.125, 0.0, 1.0 * M_PI / 1.0, 0.5 * M_PI },
   { 0.25, 2.0 * M_PI / 1.0, 0.0, 0.5 * M_PI },
#endif   
  };
int nsw = sizeof(sws) / sizeof(sinewave);

// Vertices and indices ('elements' in OpenGL parlance).
// For storing data, dynamically allocated.
// Used in different rendering modes.
Vertex *vertices = 0;
int *indices = 0;
const void **indices2D = 0;
int *counts = 0;
int n_vertices = 0, n_indices = 0;

// Global state
typedef enum { line, fill } PolygonMode;
typedef enum {
  immediate,
  storedVertices,
  storedVerticesAndIndices,
  vertexArrays,
  vertexBufferObjects,
  lastRenderMode
} RenderMode;
typedef struct {
  bool animate;
  float t, lastT;
  PolygonMode polygonMode;
  bool lighting;
  bool drawNormals;
  int width, height;
  int tess;
  int waveDim;
  int frameCount;
  float frameRate;
  float displayStatsInterval;
  int lastStatsDisplayT;
  bool displayOSD;
  bool consolePM;
  RenderMode renderMode;
} Global;
Global g = { false, 0.0, 0.0, line, false, false, 0, 0, 8, 2, 0, 0.0, 1.0, 0, false, false, immediate };

char *renderModeName[] = {
  "IM",
  "SV",
  "SV&I",
  "VA",
  "VBO",
};

// Camera state
typedef enum { inactive, rotate, pan, zoom } CameraControl;

struct {
  int lastX, lastY;
  float rotateX, rotateY;
  float scale;
  CameraControl control;
} camera = { 0, 0, 30.0, -30.0, 1.0, inactive };

// Name some colors 
color3f cyan = { 1.0, 0.0, 1.0 };
color3f white = { 1.0, 1.0, 1.0 };
color3f grey = { 0.8, 0.8, 0.8 };
color3f black = { 1.0, 1.0, 1.0};

const float milli = 1000.0;

#define BUFFER_OFFSET(i) ((void*)(i))

unsigned vbo, ibo;

void computeAndStoreSineWaveSum(sinewave sws[], int nsw, int tess, bool tessChange);

void buildVertexBufferObjects();

void panic(char *s)
{
  fprintf(stderr, "%s\n", s);
  exit(EXIT_FAILURE);
}

// Utility function 
void checkForGLerrors(int lineno)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR)
    printf("%d: %s\n", lineno, gluErrorString(error));
}

void init(void) 
{
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glEnable(GL_DEPTH_TEST);
}

void enableClientState()
{
}

void disableClientState()
{
}

void enableVertexArrays()
{
#if 0
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
#endif
}

void disableVertexArrays()
{	
#if 0
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#endif
}

void bindVertexBufferObjects()
{
}

void unbindVertexBufferObjects()
{
#if 1
	int buffer;

	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
	if (buffer != 0)
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buffer);
	if (buffer != 0)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

void enableVertexBufferObjects()
{
#if 0
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
#endif
}

void disableVertexBufferObjects()
{
#if 0
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#endif
}

void bufferData()
{
}
  
void buildVertexBufferObjects()
{
}

void reshape(int w, int h)
{
  g.width = w;
  g.height = h;
  glViewport(0, 0, (GLsizei) w, (GLsizei) h); 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -100.0, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void drawAxes(float length)
{
  glPushAttrib(GL_CURRENT_BIT);
  glBegin(GL_LINES);

  /* x axis */
  glColor3f(1.0, 0.0, 0.0);
  glVertex3f(-length, 0.0, 0.0);
  glVertex3f(length, 0.0, 0.0);

  /* y axis */
  glColor3f(0.0, 1.0, 0.0);
  glVertex3f(0.0, -length, 0.0);
  glVertex3f(0.0, length, 0.0);

  /* z axis */
  glColor3f(0.0, 0.0, 1.0);
  glVertex3f(0.0, 0.0, -length);
  glVertex3f(0.0, 0.0, length);

  glEnd();
  glPopAttrib();
}

// Draw a vector from r in direction of v scaled by s possibly normalized
void drawVector(vec3f *r, vec3f *v, float s, bool normalize, color3f *c)
{
  glPushAttrib(GL_CURRENT_BIT);
  glColor3fv((GLfloat *)c);
  glBegin(GL_LINES);
  if (normalize) {
    float mag = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x /= mag;
    v->y /= mag;
    v->z /= mag;
  }
  glVertex3fv((GLfloat *)r);
  glVertex3f(r->x + s * v->x, r->y + s * v->y, r->z + s * v->z);
  glEnd();
  glPopAttrib();
}

// Console performance meter
void consolePM()
{
  printf("frame rate (f/s):  %5.0f\n", g.frameRate);
  printf("frame time (ms/f): %5.0f\n", 1.0 / g.frameRate * 1000.0);
  printf("tesselation:       %5d\n", g.tess);
  printf("render mode:       %5s\n", renderModeName[g.renderMode]);
}

// On screen display 
void displayOSD()
{
  char buffer[30];
  char *bufp;
  int w, h;
    
  glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  /* Set up orthographic coordinate system to match the window, 
     i.e. (0,0)-(w,h) */
  w = glutGet(GLUT_WINDOW_WIDTH);
  h = glutGet(GLUT_WINDOW_HEIGHT);
  glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glViewport(0, 0, w, h);

  /* Frame rate */
  glColor3f(1.0, 1.0, 0.0);
  glRasterPos2i(10, 60);
  snprintf(buffer, sizeof buffer, "frame rate (f/s):  %5.0f", g.frameRate);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

  /* Frame time */
  glColor3f(1.0, 1.0, 0.0);
  glRasterPos2i(10, 40);
  snprintf(buffer, sizeof buffer, "frame time (ms/f): %5.0f", 1.0 / g.frameRate * milli);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

  /* Tesselation */
  glColor3f(1.0, 1.0, 0.0);
  glRasterPos2i(10, 20);
  snprintf(buffer, sizeof buffer, "tesselation:       %5d", g.tess);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

  glPopMatrix();  /* Pop modelview */
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();  /* Pop projection */
  glMatrixMode(GL_MODELVIEW);

  glPopAttrib();
}

// Calculate sinewave function and derivatives
void calcSineWave(sinewave wave, float x, float z, float t, float *y,
		  bool dvs, float *dydx, float *dydz)
{
  float angle = wave.kx * x + wave.kz * z + wave.w * t;
  *y = wave.A * sinf(angle);
  if (dvs) {
    float c = cosf(angle);
    *dydx = wave.kx * wave.A * c;
    *dydz = wave.kz * wave.A * c;
  }
}

void calcSineWaveSum(sinewave sws[], int nsw,
		     float x, float z, float t, float *y,
		     bool dvs, float *dydx, float *dydz)
{
  int i;
  float sy, sdydx, sdydz;

  sy = 0.0;
  sdydx = 0.0;
  sdydz = 0.0;
  for (i = 0; i < nsw; i++) {
    calcSineWave(sws[i], x, z, t, y, dvs, dydx, dydz);
    sy += *y;
    if (dvs) {
      sdydx += *dydx;
      sdydz += *dydz;
    }
  }
  *y = sy;
  *dydx = sdydx;
  *dydz = sdydz;
}
    

// Draw a sine wave using immediate mode
void drawSineWaveImmediate(sinewave sws[], int nsw, int tess)
{
  float stepSize = 2.0 / tess;
  vec3f r, n;
  int i, j;
  float t = g.t;

  for (i = 0; i < tess; i++) {

    // Sine wave
    glBegin(GL_TRIANGLE_STRIP);
    
    for (j = 0; j <= tess; j++) {

      // Vertex and normal
      r.x = -1.0 + i * stepSize;
      r.z = -1.0 + j * stepSize;
      calcSineWaveSum(sws, nsw, r.x, r.z, t, &r.y, g.lighting ? true : false, &n.x, &n.z);
      if (g.lighting) {
		n.x = -n.x; n.z = -n.z; n.y = 1.0;
		glNormal3fv((GLfloat *)&n);
      }
      glVertex3fv((GLfloat *)&r);

      // Next column. 
      // Vertex
      r.x += stepSize;
      calcSineWaveSum(sws, nsw, r.x, r.z, t, &r.y, true, &n.x, &n.z);
      if (g.lighting) {
		n.x = -n.x; n.z = -n.z; n.y = 1.0;
		glNormal3fv((GLfloat *)&n);
      }
      glVertex3fv((GLfloat *)&r);
    }
    glEnd();
  }

  if (g.lighting) {
    glDisable(GL_LIGHTING);
  }

  // Normals
  if (g.drawNormals) {
    for (j = 0; j <= tess; j++) {
      for (i = 0; i <= tess; i++) {
		r.x = -1.0 + i * stepSize;
		r.z = -1.0 + j * stepSize;
		calcSineWaveSum(sws, nsw, r.x, r.z, t, &r.y, true, &n.x, &n.z);
		n.x = -n.x; n.z = -n.z;	n.y = 1.0;
		drawVector(&r, &n, 0.05, true, &cyan);
      }
    }
  }
}

// Compute and store coordinates and normals for a sine wave
void computeAndStoreSineWaveSum(sinewave sws[], int nsw, int tess, bool tessChange)
{
	n_vertices = (tess + 1) * (tess + 1);
	n_indices = n_vertices * 2;
	free(vertices);
	vertices = (Vertex *)malloc(n_vertices * sizeof(Vertex));
	free(indices);
	indices = (unsigned *)malloc(n_indices * sizeof(unsigned));
  
	/* Vertices */
  	float stepSize = 2.0 / tess;
  	float t = g.t;
	for (int i = 0; i <= tess; i++) {
        for (int j = 0; j <= tess; j++) {
        	int idx = i * (tess + 1) + j;
            // Vertex and normal
            vertices[idx].r.x = -1.0 + i * stepSize;
            vertices[idx].r.z = -1.0 + j * stepSize;
            
            calcSineWaveSum(sws, nsw, vertices[idx].r.x, vertices[idx].r.z, t, 	&vertices[idx].r.y, g.lighting ? true : false, &vertices[idx].n.x, &vertices[idx].n.z);
            
            if (g.lighting) {
                vertices[idx].n.x = -vertices[idx].n.x;
                vertices[idx].n.z = -vertices[idx].n.z;
                vertices[idx].n.y = 1.0;
            }
        }
    }
    
    /* Indices */
    unsigned *idx = indices;
    for (int i = 0; i < tess; i++){
    	for (int j = 0; j <= tess; j++){
    		*idx++ = i * (tess + 1) + j;
    		*idx++ = (i + 1) * (tess + 1) + j;
    	}
    }
    
    /* Normals */	
    if (g.drawNormals){
    	for (int j = 0; j <= tess; j++){
    		for (int i = 0; i <= tess; i++){
    			int idx = j * (tess + 1) + i;
    			vertices[idx].r.x = -1.0 + i * stepSize;
    			vertices[idx].r.z = -1.0 + j * stepSize;
    			calcSineWaveSum(sws, nsw, vertices[idx].r.x, vertices[idx].r.z, t, &vertices[idx].r.y, true, &vertices[idx].n.x, &vertices[idx].n.z);
    			vertices[idx].n.x = -vertices[idx].n.x; 
    			vertices[idx].n.z = -vertices[idx].n.z;	
    			vertices[idx].n.y = 1.0;
				drawVector(&vertices[idx].r, &vertices[idx].n, 0.05, true, &cyan);
				
    		}
    	}
    }
}

// Update vertex coordinates and normals in array
void updateStoredArraySineWave(sinewave sws[], int nsw)
{
 for (int i = 0; i <= g.tess; i++) {
        for (int j = 0; j <= g.tess; j++) {
            calcSineWaveSum(sws, nsw, vertices[i * g.tess + j].r.x, vertices[i * g.tess +j].r.z, g.t, &vertices[i * g.tess + j].r.y, g.lighting ? true : false, &vertices[i * g.tess +j].n.x, &vertices[i * g.tess + j].n.z);
            if (g.lighting) {
                vertices[i * g.tess + j].n.x = -vertices[i * g.tess + j].n.x;
                vertices[i * g.tess + j].n.z = -vertices[i * g.tess + j].n.z;
                vertices[i * g.tess + j].n.y = 1.0;
            }
        }
    }
}


// Update vertex coordinates and normals in buffer
void updateBufferDataSineWave(sinewave sws[], int nsw)
{

}

// Draw a sine wave using stored vertex coordinates and normals
void drawSineWaveStoredVertices(int tess) // (1st)
{
	glPushAttrib(GL_CURRENT_BIT);
	computeAndStoreSineWaveSum(sws, nsw, tess, true);
	
    for (int i = 0; i < tess; i++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= tess; j++) {
        	int idx = i * (tess + 1) + j;
        	int nextIdx = (i + 1) * (tess + 1) + j;
            if (g.lighting)
                glNormal3fv((GLfloat *)&vertices[idx].n);
            glVertex3fv((GLfloat *)&vertices[idx].r);
            
            // Next column.
            if (g.lighting)
            	glNormal3fv((GLfloat *)&vertices[nextIdx].n);
            glVertex3fv((GLfloat *)&vertices[nextIdx].r);
        }
        glEnd();
    }
    
    if(g.lighting)
    	glDisable(GL_LIGHTING);
	
    /* Normals */
    #if 0
    if (g.drawNormals){
    	
    	#if 1
    	for (int j = 0; j <= tess; j++){
    		//glBegin(GL_LINES);
    		for (int i = 0; i <= tess; i++){
    			int idx = j * (tess + 1) + i;
    			int nextIdx = (j + 1) * (tess + 1) + i;
    			#if 0
    			glColor3f(1.0, 0.0, 1.0);
    			glVertex3fv((GLfloat *)&vertices[idx].r);
    			glVertex3f(vertices[idx].r.x + 0.05 * vertices[idx].n.x, 
    						vertices[idx].r.y + 0.05 * vertices[idx].n.y, 
    						vertices[idx].r.z + 0.05 * vertices[idx].n.z);
    			#endif
    			
    			#if 0
    			drawVector(&vertices[idx].r, &vertices[idx].n, 0.05, true, &cyan);
    			drawVector(&vertices[nextIdx].r, &vertices[nextIdx].n, 0.05, true, &cyan);
    			#endif
    		}
    		//glEnd();
    	}
    	#endif
    }
    #endif
    	
    glPopAttrib();
}

// Draw a sine wave using stored vertex coordinates and normals, and indices
void drawSineWaveStoredVerticesAndIndices(int tess) // (2nd)
{
	glPushAttrib(GL_CURRENT_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), vertices);
	
	if(g.lighting){
		computeAndStoreSineWaveSum(sws, nsw, tess, true);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, sizeof(Vertex), &grey);
	}	
	else
		glDisableClientState(GL_COLOR_ARRAY);
    	
	for (int i = 0; i < tess; i++){
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j <= tess; j++){
			int idx = i * (tess + 1) + j;
			int nextIdx = (i + 1) * (tess + 1) + j;
			if (g.lighting)
                glNormal3fv((GLfloat *)&vertices[idx].n);
			glArrayElement(idx);
			if (g.lighting)
                glNormal3fv((GLfloat *)&vertices[nextIdx].n);
			glArrayElement(nextIdx);
			
		}
		glEnd();
	}
	
	/* Normals */
	if(g.drawNormals){
		computeAndStoreSineWaveSum(sws, nsw, tess, true);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glPopAttrib();
}

// Draw a sine wave using vertex arrays and drawelements
void drawSineWaveVertexArraysDrawElements(int tess) // (3rd)
{
	glPushAttrib(GL_CURRENT_BIT);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), vertices);
	glNormalPointer(GL_FLOAT, sizeof(Vertex), vertices + 3 * sizeof(float));
	glColor3f(1.0, 1.0, 1.0);
	
	for (int i = 0; i < tess; i++){
		glDrawElements(GL_TRIANGLE_STRIP, (tess + 1) * 2, GL_UNSIGNED_INT, &indices[i * (tess + 1) * 2]);
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glPopAttrib();
}

// Draw a sine wave using vertex arrays
void drawSineWaveVertexArrays(int tess)
{
  // Strategy pattern
  drawSineWaveVertexArraysDrawElements(tess);
}

void drawSineWaveVertexBufferObjectsDrawElements(int tess) // (4th)
{
	
    //glEnableClientState(GL_COLOR_ARRAY);
    
    glPushAttrib(GL_CURRENT_BIT);
    
    //glGenBuffers(1, &vbo);
   // glGenBuffers(1, &ibo);
   	glGenBuffers(NUM_BUFFERS, buffers);
   	
   	#if 1
   	glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO]);
    glBufferData(GL_ARRAY_BUFFER, n_vertices * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), BUFFER_OFFSET(0));
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[IBO]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indices * sizeof(unsigned), indices, 						GL_STATIC_DRAW);
    #endif
    
    glColor3f(1.0, 1.0, 1.0);
    
    // Sine Wave
    for (int i = 0; i < tess; i++) {
        // Code to draw a column using a triangle strip
        glDrawElements(GL_TRIANGLE_STRIP, (tess + 1) * 2, GL_UNSIGNED_INT,
                       BUFFER_OFFSET(i * (tess + 1) * 2 * sizeof(unsigned)));
    }
    
    unbindVertexBufferObjects();
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopAttrib();
    
    
    //glDisableClientState(GL_COLOR_ARRAY);
}

void drawSineWaveVertexBufferObjects(int tess)
{
    drawSineWaveVertexBufferObjectsDrawElements(tess);
}

void drawSineWave(sinewave sw[], int nsw, int tess)
{
  glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
  if (g.lighting) {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *) &white);
    glMaterialf(GL_FRONT, GL_SHININESS, 20.0);
  } else {
    glDisable(GL_LIGHTING);
  }

  if (g.polygonMode == line)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


  //Strategy pattern
  switch (g.renderMode) {
  case immediate:
    drawSineWaveImmediate(sws, nsw, tess);
    break;
  case storedVertices:
    if (g.animate)
      computeAndStoreSineWaveSum(sws, nsw, g.tess, false);
    drawSineWaveStoredVertices(tess);
    break;
  case storedVerticesAndIndices:
    if (g.animate)
      computeAndStoreSineWaveSum(sws, nsw, g.tess, false);
    drawSineWaveStoredVerticesAndIndices(tess);
    break;
  case vertexArrays:
    if (g.animate) {
      updateStoredArraySineWave(sws, nsw);
    }
    enableVertexArrays();
    drawSineWaveVertexArrays(tess);
    disableVertexArrays();
    break;
  case vertexBufferObjects:
    if (g.animate) {
      updateBufferDataSineWave(sws, nsw);
    }
    enableVertexBufferObjects();
    drawSineWaveVertexBufferObjects(tess);
    disableVertexBufferObjects();
    break;
  default:
    break;
  }
  glPopAttrib();
}

void idle()
{
  float t, dt;

  t = glutGet(GLUT_ELAPSED_TIME) / milli;

  // Accumulate time if animation enabled
  if (g.animate) {
    dt = t - g.lastT; 
    g.t += dt;
    if (debug[d_animation])
      printf("idle: animate %f\n", g.t);
  }
  
  g.lastT = t;
  

  // Update stats, although could make conditional on a flag set interactively
  dt = (t - g.lastStatsDisplayT);
  if (dt > g.displayStatsInterval) {
    g.frameRate = g.frameCount / dt;
    if (debug[d_OSD])
      printf("dt %f framecount %d framerate %f\n", dt, g.frameCount, g.frameRate);
    g.lastStatsDisplayT = t;
    g.frameCount = 0;
  }

  glutPostRedisplay();
}

 
void displayMultiView()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  // Front view
  glPushMatrix();
  glViewport(g.width / 16.0, g.height * 9.0 / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
  drawAxes(5.0);
  drawSineWave(sws, nsw, g.tess);
  glPopMatrix();

  // Top view
  glPushMatrix();
  glViewport(g.width / 16.0, g.height / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  drawAxes(5.0);
  drawSineWave(sws, nsw, g.tess);
  glPopMatrix();

  // Left view
  glPushMatrix();
  glViewport(g.width * 9.0 / 16.0, g.height * 9.0 / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
  glRotatef(90.0, 0.0, 1.0, 0.0);
  drawAxes(5.0);
  drawSineWave(sws, nsw, g.tess);
  glPopMatrix();

  // General view
  glPushMatrix();
  glViewport(g.width * 9.0 / 16.0, g.width / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
#if 0
  glRotatef(30.0, 1.0, 0.0, 0.0);
  glRotatef(-30.0, 0.0, 1.0, 0.0);
#endif
  glRotatef(camera.rotateX, 1.0, 0.0, 0.0);
  glRotatef(camera.rotateY, 0.0, 1.0, 0.0);
  glScalef(camera.scale, camera.scale, camera.scale);
  drawAxes(5.0);
  drawSineWave(sws, nsw, g.tess);
  glPopMatrix();

  if (g.displayOSD)
    displayOSD();

  if (g.consolePM)
    consolePM();

  glutSwapBuffers();

  checkForGLerrors(__LINE__);

  g.frameCount++;
}
    
void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  glViewport(0, 0, g.width, g.height);

  // General view
  glPushMatrix();

  glRotatef(camera.rotateX, 1.0, 0.0, 0.0);
  glRotatef(camera.rotateY, 0.0, 1.0, 0.0);
  glScalef(camera.scale, camera.scale, camera.scale);
  drawAxes(5.0);

  // Draw sine wave
  drawSineWave(sws, nsw, g.tess);
  glPopMatrix();

  if (g.displayOSD)
    displayOSD();

  if (g.consolePM)
    consolePM();

  glutSwapBuffers();

  checkForGLerrors(__LINE__);

  g.frameCount++;
}
    
void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    printf("exit\n");
    exit(0);
    break;
  case 'a':
    g.animate = !g.animate;
    break;
  case 'l':
    g.lighting = !g.lighting;
    glutPostRedisplay();
    break;
  case 'm':
    printf("%d\n", g.polygonMode);
    if (g.polygonMode == line)
      g.polygonMode = fill;
    else
      g.polygonMode = line;
    glutPostRedisplay();
    break;
  case 'n':
    g.drawNormals = !g.drawNormals;
    glutPostRedisplay();
    break;
  case 'c':
    g.consolePM = !g.consolePM;
    glutPostRedisplay();
    break;
  case 'o':
    g.displayOSD = !g.displayOSD;
    glutPostRedisplay();
    break;
  case 's':
    glutDisplayFunc(display);
    glutPostRedisplay();
    break;
  case 'S':
    glutDisplayFunc(displayMultiView);
    glutPostRedisplay();
    break;
  case '+':
    g.tess *= 2;
    computeAndStoreSineWaveSum(sws, nsw, g.tess, true);
    if (g.renderMode == vertexBufferObjects)
      bufferData();
    glutPostRedisplay();
    break;
  case '-':
    g.tess /= 2;
    if (g.tess < 8)
      g.tess = 8;
    computeAndStoreSineWaveSum(sws, nsw, g.tess, true);
    if (g.renderMode == vertexBufferObjects)
      bufferData();
    glutPostRedisplay();
    break;
  case 'd':
    g.waveDim++;
    if (g.waveDim > 3)
      g.waveDim = 2;
    glutPostRedisplay();
    break;
  default:
    break;
  }
}


void keyboardSpecial(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_F1:
    g.renderMode++;
    if (g.renderMode >= lastRenderMode)
      g.renderMode = 0;
    // Moving from immediate mode means may have stale data due to animation
    if (g.renderMode == immediate + 1)
      computeAndStoreSineWaveSum(sws, nsw, g.tess, false);
    // Similarly moving to vertex buffer objects means possibly stale buffer
    // due to animation
    if (g.renderMode == vertexBufferObjects) 
      bufferData();
    glutPostRedisplay();
    break;
  default:
    break;
  }
}

void mouse(int button, int state, int x, int y)
{
  if (debug[d_mouse])
    printf("mouse: %d %d %d\n", button, x, y);

  camera.lastX = x;
  camera.lastY = y;

  if (state == GLUT_DOWN)
    switch(button) {
    case GLUT_LEFT_BUTTON:
      camera.control = rotate;
      break;
    case GLUT_MIDDLE_BUTTON:
      camera.control = pan;
      break;
    case GLUT_RIGHT_BUTTON:
      camera.control = zoom;
      break;
    }
  else if (state == GLUT_UP)
    camera.control = inactive;
}

void motion(int x, int y)
{
  float dx, dy;

  if (debug[d_mouse]) {
    printf("motion: %d %d\n", x, y);
    printf("camera.rotate: %f %f\n", camera.rotateX, camera.rotateY);
    printf("camera.scale:%f\n", camera.scale);
  }

  dx = x - camera.lastX;
  dy = y - camera.lastY;
  camera.lastX = x;
  camera.lastY = y;

  switch (camera.control) {
  case inactive:
    break;
  case rotate:
    camera.rotateX += dy;
    camera.rotateY += dx;
    break;
  case pan:
    break;
  case zoom:
    camera.scale += dy / 100.0;
    break;
  }

  glutPostRedisplay();
}



int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize (400, 400); 
  glutInitWindowPosition (100, 100);
  glutCreateWindow (argv[0]);
  init();
  glutDisplayFunc(display); 
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(keyboardSpecial);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutMainLoop();
  return 0;

}

