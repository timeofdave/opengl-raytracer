#include "Object.h"

Triangle::Triangle(point3 p0, point3 p1, point3 p2, point3 n) {
	vertices[0] = p0;
	vertices[1] = p1;
	vertices[2] = p2;
	normal = n;
}


Object::Object(int theType)
{
	this->type = theType;
}

Light::Light(int theType)
{
	this->type = theType;
}


