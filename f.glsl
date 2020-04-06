#version 150

//in float u;
out vec4 out_colour;
uniform vec2 Window;
uniform sampler1D tex_sampler;

void main() 
{ 
  //out_colour.rgb = texture( tex_sampler, u ).rgb;
  float aspectRatio = Window.x / Window.y;
  float yPos = (gl_FragCoord.y / Window.y) * 2 - 1;
  float xPos = (gl_FragCoord.x / Window.x) * aspectRatio * 2 - (aspectRatio);

  vec3 e = vec3(0, 0, 0);
  vec3 s = vec3(xPos, yPos, -1.0);

  //colour = vec3(1, 0, 0);
  //if (trace(e, s, colour)) {
  //  out_colour.rgb = colour;
  //}
  
  // Debugging the view rect
  out_colour.r = xPos;
  out_colour.g = 0;
  if (abs(xPos) > 0.9) { out_colour.g = 1; }
  if (abs(yPos) > 0.9) { out_colour.g = 1; }
  out_colour.b = yPos;

  out_colour.a = 1;
}
