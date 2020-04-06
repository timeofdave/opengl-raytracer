#version 150

//in float u;
out vec4 out_colour;
uniform vec2 Window;
//uniform sampler1D tex_sampler;
uniform vec3 data[6];

float debug = 1234567.0; // flag value, means not debugging
float debugMin = -1.0;
float debugMax = 1.0; // set these to a reasonable range

bool trace(vec3 e, vec3 s, out vec3 colour);
void debugRed();
void debugViewRect(float xPos, float yPos);

void main() 
{ 
  //out_colour.rgb = texture( tex_sampler, u ).rgb;
  float aspectRatio = Window.x / Window.y;
  float yPos = (gl_FragCoord.y / Window.y) * 2 - 1;
  float xPos = (gl_FragCoord.x / Window.x) * aspectRatio * 2 - (aspectRatio);

  vec3 e = vec3(0, 0, 0);
  vec3 s = vec3(xPos, yPos, -1.0);

  vec3 colour = vec3(1, 0, 0);
  if (trace(e, s, colour)) {
   out_colour.rgb = colour;
  }

  debug = data[0].y;
  
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