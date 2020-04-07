#version 150

// --------------------- Constants
const int NUM_OBJECTS = 20;
const int NUM_LIGHTS = 10;
const int SPACE_GEOMETRY = 1000;
const int SPACE_MATERIALS = 100;


// --------------------- Uniforms
out vec4 out_colour;
uniform vec2 Window;
uniform int objectIds[NUM_OBJECTS];
uniform int lightIds[NUM_LIGHTS];
uniform vec3 geometry[SPACE_GEOMETRY];
uniform vec3 materials[SPACE_MATERIALS];

// --------------------- Debug Variables
float debug = 1234567.0; // flag value, means not debugging
float debugMin = -10.0;
float debugMax = 0.0; // set these to a reasonable range


// --------------------- Function Headers
bool trace(vec3 e, vec3 s, out vec3 colour);
void debugRed();
void debugViewRect(float xPos, float yPos);





void main() { 
  float aspectRatio = Window.x / Window.y;
  float yPos = (gl_FragCoord.y / Window.y) * 2 - 1;
  float xPos = (gl_FragCoord.x / Window.x) * aspectRatio * 2 - (aspectRatio);

  vec3 e = vec3(0, 0, 0);
  vec3 s = vec3(xPos, yPos, -1.0);

  vec3 colour = vec3(1, 0, 0);
  if (trace(e, s, colour)) {
    out_colour.rgb = colour;
  }

  debug = geometry[2].z;
  
  debugViewRect(xPos, yPos);
  debugRed();
}


bool trace(vec3 e, vec3 s, out vec3 colour) {
  return false;
}


void debugRed() {
  if (debug != 1234567.0) {
    float red = (debug - debugMin) / (debugMax - debugMin);
    out_colour.r = red;
    out_colour.g = 0;
    out_colour.b = 0;
    out_colour.a = 0;
  }
}


void debugViewRect(float xPos, float yPos) {
  out_colour.r = xPos;
  out_colour.g = 0;
  if (abs(xPos) > 0.9) { out_colour.g = 1; }
  if (abs(yPos) > 0.9) { out_colour.g = 1; }
  out_colour.b = yPos;
  out_colour.a = 1;
}