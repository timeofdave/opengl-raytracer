#pragma once
#include "common.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>


typedef glm::vec4  color4;
typedef glm::vec4  point4;

class Triangle {
public:
	point3 vertices[3];
	point3 normal;

	Triangle(point3 p0, point3 p1, point3 p2, point3 n);
};

class Object {
public:
	int type;

	point3 pos = point3(0.f, 0.f, 0.f);
	float radius = 0.f;
	point3 normal = point3(0.f, 0.f, 0.f);
	std::vector<Triangle *> tris;

	colour3 ambient = colour3(0.f, 0.f, 0.f);
	colour3 diffuse = colour3(0.f, 0.f, 0.f);
	colour3 specular = colour3(0.f, 0.f, 0.f);
	float shininess = 0.f;
	colour3 reflective = colour3(0.f, 0.f, 0.f);
	colour3 transmissive = colour3(0.f, 0.f, 0.f);
	float refraction = 0.f;

	Object(int theType);
};

class Light {
public:
	int type;
	colour3 colour;

	point3 pos;
	float cutoff;
	point3 direction;
	float radius;

	Light(int theType);
};