// Assignment 2 Question 1

#include "common.h"
#include "raytracer.h"
#include "packer.h"

#include <iostream>
#define M_PI 3.14159265358979323846264338327950288
#include <cmath>

#include <glm/glm.hpp>
#include <glm\gtc\type_ptr.hpp>

const char *WINDOW_TITLE = "Ray Tracing";
const double FRAME_RATE_MS = 1;

const int NUM_OBJECTS = 20;
const int NUM_LIGHTS = 10;
const int SPACE_GEOMETRY = 1000;
const int SPACE_MATERIALS = 100;

GLuint Window;
int vp_width, vp_height;

point3 eye;
float d = 1;

point3 vertices[6] = {
	point3(-1.0,  1.0,  1.0),
	point3(-1.0, -1.0,  1.0),
	point3(1.0,  -1.0,  1.0),
	point3(-1.0, 1.0,  1.0),
	point3(1.0, -1.0, 1.0),
	point3(1.0,  1.0, 1.0)
};

int objectIds[NUM_OBJECTS];
int lightIds[NUM_LIGHTS];
point3 geometry[SPACE_GEOMETRY];
point3 materials[SPACE_MATERIALS];

//----------------------------------------------------------------------------

point3 s(int x, int y) {
	float aspect_ratio = (float)vp_width / vp_height;
	float h = d * (float)tan((M_PI * fov) / 180.0 / 2.0);
	float w = h * aspect_ratio;
   
	float top = h;
	float bottom = -h;
	float left = -w;
	float right = w;
   
	float u = left + (right - left) * (x + 0.5f) / vp_width;
	float v = bottom + (top - bottom) * (y + 0.5f) / vp_height;
   
	return point3(u, v, -d);
}

//----------------------------------------------------------------------------

// OpenGL initialization
void init(char *fn) {
	choose_scene(fn);
	pack_scene();
   
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW );

	// Load shaders and use the resulting shader program
	GLuint program = InitShader( "v.glsl", "f.glsl" );
	glUseProgram( program );

	// set up vertex arrays
	GLuint vPos = glGetAttribLocation( program, "vPos" );
	glEnableVertexAttribArray( vPos );
	glVertexAttribPointer( vPos, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	Window = glGetUniformLocation( program, "Window" );

	glClearColor( 0.7, 0.7, 0.8, 1 );


	glUniform1iv(glGetUniformLocation(program, "objectIds"), NUM_OBJECTS, &objectIds[0]);
	glUniform1iv(glGetUniformLocation(program, "lightIds"), NUM_LIGHTS, &lightIds[0]);
	glUniform3fv(glGetUniformLocation(program, "geometry"), SPACE_GEOMETRY, glm::value_ptr(geometry[0]));
	glUniform3fv(glGetUniformLocation(program, "materials"), SPACE_MATERIALS, glm::value_ptr(materials[0]));
}

//----------------------------------------------------------------------------


void display(void) {
	std::cout << "--- Next Frame ---" << std::endl;

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glFlush();
	glFinish();
	glutSwapBuffers();
}


//----------------------------------------------------------------------------

void keyboard( unsigned char key, int x, int y ) {
	switch( key ) {
	case 033: // Escape Key
	case 'q': case 'Q':
		exit( EXIT_SUCCESS );
		break;
	case ' ':
		break;
	}
}

//----------------------------------------------------------------------------

void mouse( int button, int state, int x, int y ) {
	y = vp_height - y - 1;
	if ( state == GLUT_DOWN ) {
		switch( button ) {
		case GLUT_LEFT_BUTTON:
			colour3 c;
			point3 uvw = s(x, y);
			std::cout << std::endl;
			std::cout << "--------------------------- Raycast -----------------------------\n";
			if (trace(eye, uvw, c, true, 0, true)) {
				//std::cout << "HIT @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
				//std::cout << "      colour = ( " << c.r << "," << c.g << "," << c.b << " )\n";
			} else {
				//std::cout << "MISS @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------

void update( void ) {
}

//----------------------------------------------------------------------------

void reshape( int width, int height ) {
	glViewport( 0, 0, width, height );

	// GLfloat aspect = GLfloat(width)/height;
	// glm::mat4  projection = glm::ortho( -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f );
	// glUniformMatrix4fv( Projection, 1, GL_FALSE, glm::value_ptr(projection) );
	vp_width = width;
	vp_height = height;
	glUniform2f( Window, width, height );
	//drawing_y = 0;
}
