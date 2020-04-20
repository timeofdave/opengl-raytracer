// Assignment 2 Question 1

#include "common.h"
#include "raytracer.h"
#include "packer.h"

#include <iostream>
#define M_PI 3.14159265358979323846264338327950288
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

const char *WINDOW_TITLE = "Ray Tracing";
const double FRAME_RATE_MS = 1;

const int NUM_OBJECTS = 20;
const int NUM_LIGHTS = 10;
const int SPACE_GEOMETRY = 1000;
const int SPACE_MATERIALS = 100;
const float MOVE_SPEED = 6.0;
const float ROTATE_SPEED = 70.0;
const float GRAVITY = -9.8;

GLuint Window;
GLuint program;
GLuint ViewTrans;
GLuint BounceTrans;
GLuint SpinTrans;

int vp_width, vp_height;
int numObjects;
int numLights;

// User-controllable uniforms
point3 bouncePos(0, 2, 0);
point3 bounceVelo(0, 0, 0);
point3 spinTheta(0, 0, 0);
point3 eyePos;
point3 eyeTheta;
bool SHOW_LIGHTS = false;
int ALIAS_RAYS = 1; // VALID VALUES (1, 4)
bool LIGHT_ATTENUATION = true;
bool AREA_SHADOWS = true;

float d = 1;
float fps = 100.0; // starting point
void fpsMeter();
void keyboardWindows();
void bounceTransform();
void spinTransform();
void resendSettings();

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
	eyePos = point3(0, 0, 0);
   
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
	program = InitShader( "v.glsl", "f.glsl" );
	glUseProgram( program );

	// set up vertex arrays
	GLuint vPos = glGetAttribLocation( program, "vPos" );
	glEnableVertexAttribArray( vPos );
	glVertexAttribPointer( vPos, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	Window = glGetUniformLocation( program, "Window" );
	ViewTrans = glGetUniformLocation( program, "ViewTrans" );
	BounceTrans = glGetUniformLocation( program, "BounceTrans" );
	SpinTrans = glGetUniformLocation( program, "SpinTrans" );

	glClearColor( 0.7, 0.7, 0.8, 1 );

	glUniform1i(glGetUniformLocation(program, "numObjects"), numObjects);
	glUniform1i(glGetUniformLocation(program, "numLights"), numLights);
	//glUniform3f(glGetUniformLocation(program, "eyePos"), eye.x, eye.y, eye.z);
	glUniform1iv(glGetUniformLocation(program, "objectIds"), NUM_OBJECTS, &objectIds[0]);
	glUniform1iv(glGetUniformLocation(program, "lightIds"), NUM_LIGHTS, &lightIds[0]);
	glUniform3fv(glGetUniformLocation(program, "geometry"), SPACE_GEOMETRY, glm::value_ptr(geometry[0]));
	glUniform3fv(glGetUniformLocation(program, "materials"), SPACE_MATERIALS, glm::value_ptr(materials[0]));
}

//----------------------------------------------------------------------------


void display(void) {
	fpsMeter();
	keyboardWindows();

	// Camera Transform
	const glm::vec3 viewer_pos(eyePos.x, eyePos.y, eyePos.z);
	glm::mat4 trans, rot, model_view;
	trans = glm::translate(trans, viewer_pos);
	rot = glm::rotate(rot, glm::radians(eyeTheta.x), glm::vec3(1, 0, 0));
	rot = glm::rotate(rot, glm::radians(eyeTheta.y), glm::vec3(0, 1, 0));
	rot = glm::rotate(rot, glm::radians(eyeTheta.z), glm::vec3(0, 0, 1));
	model_view = trans * rot;
	glUniformMatrix4fv(ViewTrans, 1, GL_FALSE, glm::value_ptr(model_view));
	//glUniform3f(glGetUniformLocation(program, "eyePos"), eye.x, eye.y, eye.z);

	bounceTransform();
	spinTransform();

	resendSettings();

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glFlush();
	glFinish();
	glutSwapBuffers();
}

void resendSettings() {
	glUniform1i(glGetUniformLocation(program, "SHOW_LIGHTS"), SHOW_LIGHTS);
	glUniform1i(glGetUniformLocation(program, "ALIAS_RAYS"), ALIAS_RAYS);
	glUniform1i(glGetUniformLocation(program, "LIGHT_ATTENUATION"), LIGHT_ATTENUATION);
	glUniform1i(glGetUniformLocation(program, "AREA_SHADOWS"), AREA_SHADOWS);
}

void bounceTransform() {
	// Physics
	bouncePos += bounceVelo / fps;

	if (bouncePos.y < -1.0) {
		bounceVelo = -bounceVelo;
		bouncePos += bounceVelo / fps;
	}

	bounceVelo.y += GRAVITY / fps;

	// Send to GPU
	const glm::vec3 model_pos(bouncePos.x, bouncePos.y, bouncePos.z);
	glm::mat4 trans, rot, model_view;
	trans = glm::translate(trans, model_pos);
	//rot = glm::rotate(rot, glm::radians(eyeTheta.x), glm::vec3(1, 0, 0));
	//rot = glm::rotate(rot, glm::radians(eyeTheta.y), glm::vec3(0, 1, 0));
	//rot = glm::rotate(rot, glm::radians(eyeTheta.z), glm::vec3(0, 0, 1));
	model_view = trans * rot;
	glUniformMatrix4fv(BounceTrans, 1, GL_FALSE, glm::value_ptr(model_view));

}

void spinTransform() {
	// Physics
	spinTheta.y += 0.4;

	// Send to GPU
	const glm::vec3 model_pos(0, 0, -5);
	glm::mat4 trans;
	trans = glm::translate(trans, model_pos);
	trans = glm::rotate(trans, glm::radians(spinTheta.x), glm::vec3(1, 0, 0));
	trans = glm::rotate(trans, glm::radians(spinTheta.y), glm::vec3(0, 1, 0));
	trans = glm::rotate(trans, glm::radians(spinTheta.z), glm::vec3(0, 0, 1));
	trans = glm::translate(trans, -model_pos);
	glUniformMatrix4fv(SpinTrans, 1, GL_FALSE, glm::value_ptr(trans));
}

//----------------------------------------------------------------------------

glm::vec3 cameraRotateVector(glm::vec3 v) {
	glm::vec4 forward4(v.x, v.y, v.z, 1);
	glm::mat4 rot;

	rot = glm::rotate(rot, glm::radians(eyeTheta.x), glm::vec3(1, 0, 0));
	rot = glm::rotate(rot, glm::radians(eyeTheta.y), glm::vec3(0, 1, 0));
	rot = glm::rotate(rot, glm::radians(eyeTheta.z), glm::vec3(0, 0, 1));
	forward4 = rot * forward4;
	glm::vec3 forward(forward4.x, forward4.y, forward4.z);

	return forward;
}

void keyboardWindows() {
	if (GetKeyState(VK_LWIN) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		return; // This allows for WIN+SHIFT+S screenshots
	}
	if (GetKeyState('W') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos -= cameraRotateVector(glm::vec3(0, 0, 1)) * (MOVE_SPEED / fps);
	}
	if (GetKeyState('S') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos += cameraRotateVector(glm::vec3(0, 0, 1)) * (MOVE_SPEED / fps);
	}
	if (GetKeyState('A') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos -= cameraRotateVector(glm::vec3(1, 0, 0)) * (MOVE_SPEED / fps);
	}
	if (GetKeyState('D') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos += cameraRotateVector(glm::vec3(1, 0, 0)) * (MOVE_SPEED / fps);
	}
	if (GetKeyState('Q') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.y += ROTATE_SPEED / fps;
	}
	if (GetKeyState('E') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.y -= ROTATE_SPEED / fps;
	}
	if (GetKeyState(VK_UP) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.x -= ROTATE_SPEED / fps;
	}
	if (GetKeyState(VK_DOWN) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.x += ROTATE_SPEED / fps;
	}
	if (GetKeyState(VK_LEFT) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.z += ROTATE_SPEED / fps / 4;
	}
	if (GetKeyState(VK_RIGHT) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyeTheta.z -= ROTATE_SPEED / fps / 4;
	}
	if (GetKeyState(VK_LSHIFT) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos += cameraRotateVector(glm::vec3(0, 1, 0)) * (MOVE_SPEED / fps);
	}
	if (GetKeyState(VK_LCONTROL) & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		eyePos -= cameraRotateVector(glm::vec3(0, 1, 0)) * (MOVE_SPEED / fps);
	}
}

void keyboard( unsigned char key, int x, int y ) {
	switch( key ) {
	case 033: // Escape Key
	//case 'q': case 'Q':
		exit( EXIT_SUCCESS );
		break;
	case '1':
		AREA_SHADOWS = !AREA_SHADOWS;
		break;
	case '2':
		SHOW_LIGHTS = !SHOW_LIGHTS;
		break;
	case '3':
		ALIAS_RAYS = (ALIAS_RAYS == 1) ? 4 : 1; // VALID VALUES (1, 4)
		break;
	case '4':
		LIGHT_ATTENUATION = !LIGHT_ATTENUATION;
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
			if (trace(eyePos, uvw, c, true, 0, true)) {
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



int before = 0;
int frameCount = 0;
void fpsMeter()
{
	if (frameCount == 0) {
		before = GetTickCount();
	}
	frameCount++;

	int now = GetTickCount();

	if (now - before > 200) {
		fps = frameCount / ((now - before) * 0.001);
		printf("FPS: %0.1f \r", fps); // Regular output

		printf("\n");
		frameCount = 0;
		before += 200; // = now;
	}

}
