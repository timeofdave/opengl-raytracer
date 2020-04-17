// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"
#include "Object.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

using json = nlohmann::json;

const char* PATH = "scenes/";
const float EPSILON = 0.0001f;
const float ANTI_ACNE = 0.001f;
const int RECURSION_LIMIT = 5;
const colour3 ZEROS = colour3(0, 0, 0);

double fov = 60;
colour3 background_colour(0, 0, 0);

json scene;

std::vector<Object *> objects;
std::vector<Light *> lights;


/****************************************************************************/

// here are some potentially useful utility functions

std::string str(point3 vec) {
	return "( " + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + " )";
}

json find(json& j, const std::string key, const std::string value) {
	json::iterator it;
	for (it = j.begin(); it != j.end(); ++it) {
		if (it->find(key) != it->end()) {
			if ((*it)[key] == value) {
				return *it;
			}
		}
	}
	return json();
}

glm::vec3 vector_to_vec3(const std::vector<float>& v) {
	return glm::vec3(v[0], v[1], v[2]);
}



int getIntType(std::string name) {
	int type = 0;

	if (name == "directional") {
		type = 1;
	}
	else if (name == "point") {
		type = 2;
	}
	else if (name == "spot") {
		type = 3;
	}

	else if (name == "plane") {
		type = 1;
	}
	else if (name == "mesh") {
		type = 2;
	}

	return type;
}

/****************************************************************************/

void populateObjects() {
	json& objectsJ = scene["objects"];

	for (json::iterator it = objectsJ.begin(); it != objectsJ.end(); ++it) {

		json& object = *it;
		json& material = object["material"];
		Object* obj = new Object(getIntType(object["type"]));

		if (object.find("position") != object.end()) {
			obj->pos = vector_to_vec3(object["position"]);
		}
		if (object.find("normal") != object.end()) {
			obj->normal = vector_to_vec3(object["normal"]);
		}
		if (object.find("radius") != object.end()) {
			obj->radius = float(object["radius"]);
		}
		if (object.find("triangles") != object.end()) {

			for (json::iterator tryit = object["triangles"].begin(); tryit != object["triangles"].end(); ++tryit) {
				json& triangle = *tryit;

				point3 A = vector_to_vec3(triangle[0]);
				point3 B = vector_to_vec3(triangle[1]);
				point3 C = vector_to_vec3(triangle[2]);
				point3 N = glm::cross(B - A, C - A);

				Triangle* tri = new Triangle(A, B, C, N);

				obj->tris.push_back(tri);
			} // for each triangle
		}

		// Material properties
		if (material.find("ambient") != material.end()) {
			obj->ambient = vector_to_vec3(material["ambient"]);
		}
		if (material.find("diffuse") != material.end()) {
			obj->diffuse = vector_to_vec3(material["diffuse"]);
		}
		if (material.find("specular") != material.end()) {
			obj->specular = vector_to_vec3(material["specular"]);
		}
		if (material.find("shininess") != material.end()) {
			obj->shininess = float(material["shininess"]);
		}
		if (material.find("reflective") != material.end()) {
			obj->reflective = vector_to_vec3(material["reflective"]);
		}
		if (material.find("transmissive") != material.end()) {
			obj->transmissive = vector_to_vec3(material["transmissive"]);
		}
		if (material.find("refraction") != material.end()) {
			obj->refraction = float(material["refraction"]);
		}

		objects.push_back(obj);
	}
}

void populateLights() {
	json& lightsJ = scene["lights"];

	for (json::iterator it = lightsJ.begin(); it != lightsJ.end(); ++it) {

		json& light = *it;
		Light* lite = new Light(getIntType(light["type"]));

		if (light.find("position") != light.end()) {
			lite->pos = vector_to_vec3(light["position"]);
		}
		if (light.find("direction") != light.end()) {
			lite->direction = vector_to_vec3(light["direction"]);
		}
		if (light.find("cutoff") != light.end()) {
			lite->cutoff = float(light["cutoff"]);
		}
		if (light.find("color") != light.end()) {
			lite->colour = vector_to_vec3(light["color"]);
		}
		if (light.find("radius") != light.end()) {
			lite->radius = float(light["radius"]);
		}
		lights.push_back(lite);
	}
}


void choose_scene(char const* fn) {
	if (fn == NULL) {
		std::cout << "Using default input file " << PATH << "c.json\n";
		fn = "c";
	}

	std::cout << "Loading scene " << fn << std::endl;

	std::string fname = PATH + std::string(fn) + ".json";
	std::fstream in(fname);
	if (!in.is_open()) {
		std::cout << "Unable to open scene file " << fname << std::endl;
		exit(EXIT_FAILURE);
	}

	in >> scene;

	json camera = scene["camera"];
	// these are optional parameters (otherwise they default to the values initialized earlier)
	if (camera.find("field") != camera.end()) {
		fov = camera["field"];
		std::cout << "Setting fov to " << fov << " degrees.\n";
	}
	if (camera.find("background") != camera.end()) {
		background_colour = vector_to_vec3(camera["background"]);
		std::cout << "Setting background colour to " << glm::to_string(background_colour) << std::endl;
	}

	populateObjects();
	populateLights();
}




void printPickingInfo(bool hit, bool pick, int recursionLevel, float& dist, int& hitObj, int& hitTri, point3 clr) {
	if (pick && recursionLevel == 0) {
		if (hit) {
			point3 dif = objects[hitObj]->diffuse;
			std::cout << "Raycast hit object " << hitObj << " at a distance of " << dist << "\n";
			std::cout << "      Object's diffuse colour: ( " << dif.r << ", " << dif.g << ", " << dif.b << " )\n";
			std::cout << "      Final output colour:     ( " << clr.r << ", " << clr.g << ", " << clr.b << " )\n";
		}
		else {
			std::cout << "Raycast missed all objects in scene." << "\n";
		}
	}
}

// Refraction debug printing
void debugPrintRefraction(point3 norm, point3 vEye, point3 V, float insideSqrt, float indexInc, float indexRef, bool pick) {
	if (pick) {
		float angleOfIncidence = glm::asin(glm::length(glm::cross(vEye, norm)));
		std::cout << "  vEye = " << str(vEye) << "\n";
		std::cout << "  norm = " << str(norm) << "\n";
		std::cout << "  angleOfIncidence = " << glm::degrees(angleOfIncidence) << "\n";
		if (insideSqrt < 0) {
			std::cout << "  <<<<<<< Total Internal Reflection >>>>>>>" << "\n";
			point3 R = glm::normalize(2.f * glm::dot(norm, V) * norm - V); // Reflection direction
			std::cout << "  R = " << str(R) << "\n";
		}
		else {
			point3 numerator1 = indexInc * (vEye - norm * glm::dot(vEye, norm));
			point3 R = glm::normalize((numerator1 / indexRef) - norm * glm::sqrt(insideSqrt));
			std::cout << "  R = " << str(R) << "\n";
			float angleOfRefraction = glm::asin(glm::length(glm::cross(R, -norm)));
			std::cout << "  angleOfRefraction = " << glm::degrees(angleOfRefraction) << "\n";
		}
	}
}

void debugPrintHit(Object* object, int indexOfClosest, point3 N, point3 P, bool pick) {
	if (pick) {
		std::cout << "Hit object " << indexOfClosest << " (Type " << object->type << ") at " << str(P) << "\n";
	}
}

void debugBreakpoints(point3 P, point3 s, point3 V, int recursionLevel) {

	// DEBUG center of screen
	if (abs(s.x) < 0.01 && abs(s.y) < 0.01 && recursionLevel == 0) {
		s = s;
	}
	// DEBUG left side of screen
	if (s.x > -0.31 && s.x < -0.3 && abs(s.y) < 0.01 && recursionLevel == 0) {
		s = s;
	}
	// DEBUG bottom of screen
	if (s.y > 0.3 && s.y < 0.31 && abs(s.x) < 0.01 && recursionLevel == 0) {
		s = s;
	}
	// DEBUG bottom of screen
	if (s.y > -0.31 && s.y < -0.3 && abs(s.x) < 0.01 && recursionLevel == 0) {
		s = s;
	}
}




/****************************************************************************/
/******************************* Ray Tracing ********************************/
/****************************************************************************/


float calcPlaneDistance(point3 A, point3 N, point3 d, point3 e) {
	float denom = glm::dot(N, d);
	float t = 0.f;

	if (denom > 0) {
		t = glm::dot(N, A - e) / denom;
		if (t > 0) {
			t = t;
		}
	}
	if (denom < 0) {
		t = glm::dot(N, A - e) / denom;
	}
	return t;
}

float acneThreshold(point3 N, point3 d) {
	float acneThreshold = ANTI_ACNE;
	float angleOfIncidenceCos = glm::dot(N, d);

	if (angleOfIncidenceCos > 0) {
		acneThreshold = ANTI_ACNE / angleOfIncidenceCos;
	}

	return acneThreshold;
}

bool getIntersection(const point3& e, const point3& d, float& dist, int& indexOfClosest, int& indexOfTriangle) {

	for (int i = 0; i < objects.size(); i++) {
		Object* object = objects[i];

		if (object->type == SPHERE) {
			point3 pos = object->pos;
			point3 emc = e - pos;
			float r = float(object->radius);

			float discriminant = glm::dot(d, emc) * glm::dot(d, emc) - glm::dot(d, d) * (glm::dot(emc, emc) - r * r);
			float t = FLT_MAX;

			if (discriminant >= 0.f) {
				// One or two intersections, I don't think I care which.
				float t1 = (glm::dot(-d, emc) + glm::sqrt(discriminant)) / glm::dot(d, d);
				float t2 = (glm::dot(-d, emc) - glm::sqrt(discriminant)) / glm::dot(d, d);
				t1 = (t1 > ANTI_ACNE) ? t1 : FLT_MAX;
				t2 = (t2 > ANTI_ACNE) ? t2 : FLT_MAX;
				t = std::min(t1, t2);

				if (t < dist) {
					dist = t;
					indexOfClosest = i;
				}
			}
		}
		else if (object->type == PLANE) {
			point3 A = object->pos;
			point3 N = object->normal;

			float t = calcPlaneDistance(A, N, d, e);

			if (t > acneThreshold(N, d)) { // hit the plane
				if (t < dist) {
					dist = t;
					indexOfClosest = i;
				}
			}
		}
		else if (object->type == MESH) {

			for (int j = 0; j < object->tris.size(); j++) {
				Triangle* triangle = object->tris[j];

				point3 A = triangle->vertices[0];
				point3 B = triangle->vertices[1];
				point3 C = triangle->vertices[2];
				point3 N = triangle->normal;

				float t = calcPlaneDistance(A, N, d, e);
				point3 X = e + (t * d);

				float inA = glm::dot(glm::cross(B - A, X - A), N);
				float inB = glm::dot(glm::cross(C - B, X - B), N);
				float inC = glm::dot(glm::cross(A - C, X - C), N);

				if (t > acneThreshold(N, d) && inA > 0.f && inB > 0.f && inC > 0.f) { // hit front of triangle
					if (t < dist) {
						dist = t;
						indexOfClosest = i;
						indexOfTriangle = j;
					}
				}
			} // for each triangle
		}
	} // for each object

	return (indexOfClosest >= 0);
}

// ------------------- SHADOW CHECK -----------------------
// Check if the point P is in shadow from this light.
// Cast a ray from the light to the point P.
bool checkIfInShadow(point3 P, Light *light, point3 lightPos, int indexOfClosest, int indexOfTriangle) {
	bool isInShadow = false;

	if (light->type > AMBIENT) {
		float distL = FLT_MAX;
		int indexObjL = -1;
		int indexTriL = -1;
		point3 shadowRay = (P - lightPos) / 100.f;
		bool hit = getIntersection(lightPos, shadowRay, distL, indexObjL, indexTriL);

		if (hit) { // should always hit, but there's always room for floating point error.
			point3 shadowP = lightPos + (distL * shadowRay); // Point of intersection

			//if (glm::distance(P, shadowP) > 0.001) {
			if (indexObjL != indexOfClosest || indexTriL != indexOfTriangle) {
				isInShadow = true; // Pretend this light doesn't exist.
			}
		}
	}
	return isInShadow;
}


bool determineLightDirection(point3 P, Light *light, point3& L, point3& lightPos) {
	bool isVisible = true;

	if (light->type == DIRECTIONAL) {
		L = glm::normalize(-1.0f * light->direction);
		lightPos = P + (100.f * L); // Has to be further away than any object.
	}
	else if (light->type == POINT_LIGHT) {
		lightPos = light->pos;
		L = glm::normalize(lightPos - P);
	}
	else if (light->type == SPOT) {
		lightPos = light->pos;
		L = glm::normalize(lightPos - P);
		point3 lightFacing = glm::normalize(light->direction);
		float cutoff = glm::radians(float(light->cutoff));
		float dotProduct = glm::dot(L, -lightFacing);

		if (dotProduct < glm::cos(cutoff)) {
			isVisible = false; // Just pretend this light doesn't exist.
		}
	}
	return isVisible;
}


// Calculate lighting equation at the hit point.
colour3 phongIllumination(Object* object, Light* light, point3 N, point3 L, point3 V) {
	colour3 total = ZEROS;

	// Ambient component
	if (object->ambient != ZEROS && light->type == AMBIENT) {
		total += light->colour * object->ambient;
	}

	// Diffuse component
	if (object->diffuse != ZEROS && light->type > AMBIENT) {
		float dotProduct = glm::dot(N, L);
		total += light->colour * object->diffuse * glm::max(0.f, dotProduct);
	}

	// Specular component
	if (object->specular != ZEROS && light->type > AMBIENT) {
		point3 R = glm::normalize(2.f * glm::dot(N, L) * N - L); // Reflection direction
		float dotProduct = glm::dot(R, V);

		if (dotProduct > 0) {
			float shine = glm::pow(dotProduct, object->shininess);
			total += light->colour * object->specular * shine;
		}
	}

	return total;
}

point3 calcNormal(Object *object, point3 P, int indexOfTriangle) {
	point3 N = ZEROS;
	
	if (object->type == SPHERE) {
		point3 center = object->pos;
		N = glm::normalize(P - center);
	}
	if (object->type == PLANE) {
		N = glm::normalize(object->normal);
	}
	if (object->type == MESH) {
		Triangle* triangle = object->tris[indexOfTriangle];
		N = glm::normalize(triangle->normal);
	}
	return N;
}

colour3 calcReflection(Object* object, point3 P, point3 N, point3 V, bool outside, bool pick, int recursionLevel) {
	colour3 result = ZEROS;

	if (true && object->reflective != ZEROS && recursionLevel < RECURSION_LIMIT && outside) {
		point3 R = glm::normalize(2.f * glm::dot(N, V) * N - V); // Reflection direction

		colour3 colourRefl;
		if (trace(P, P + R, colourRefl, pick, recursionLevel + 1, outside)) {
			result += colourRefl * object->reflective;
		}
	}

	return result;
}

colour3 calcTransmission(Object* object, point3 P, point3 V, colour3 total, bool outside, bool pick, int recursionLevel) {
	colour3 result = total;

	if (true && object->transmissive != ZEROS && object->refraction == 0.f && recursionLevel < RECURSION_LIMIT) {

		bool goingOutside = (object->type == PLANE) ? true : !outside;

		point3 R = -V;
		colour3 colourRefr;

		if (trace(P, P + R, colourRefr, pick, recursionLevel + 1, goingOutside)) {
			if (outside) {
				result = total * (1.f - object->transmissive) + colourRefr * object->transmissive;
			}
			else {
				result = colourRefr * object->transmissive;
			}
		}
	}

	return result;
}

colour3 calcRefraction(Object* object, point3 P, point3 N, point3 V, 
					   colour3 total, bool outside, bool pick, int recursionLevel) {
	colour3 result = total;

	if (true && object->transmissive != ZEROS && object->refraction != 0.f && recursionLevel < RECURSION_LIMIT) {
		point3 vEye = -V;
		point3 norm = (outside) ? N : -N;
		bool goingOutside = (object->type == PLANE) ? true : !outside;

		float indexInc = (outside) ? 1.f : object->refraction;
		float indexRef = (goingOutside) ? 1.f : object->refraction;

		float numeratorPart2 = (1 - glm::pow(glm::dot(vEye, norm), 2));
		float indicesRatio = (indexInc * indexInc) / (indexRef * indexRef);
		float insideSqrt = 1 - indicesRatio * numeratorPart2;

		debugPrintRefraction(norm, vEye, V, insideSqrt, indexInc, indexRef, pick);

		if (insideSqrt < 0) { // Total Internal Reflection
			point3 R = glm::normalize(2.f * glm::dot(norm, V) * norm - V); // Reflection direction

			colour3 colourRefl;
			if (trace(P, P + R, colourRefl, pick, recursionLevel + 1, outside)) {
				result = total * (1.f - object->transmissive) + colourRefl * object->transmissive;
			}
		}
		else { // Refraction
			point3 numerator1 = indexInc * (vEye - norm * glm::dot(vEye, norm));
			point3 R = glm::normalize((numerator1 / indexRef) - norm * glm::sqrt(insideSqrt));

			colour3 colourRefr;
			trace(P, P + R, colourRefr, pick, recursionLevel + 1, goingOutside);

			result = total * (1.f - object->transmissive) + colourRefr * object->transmissive;
		}
	}
	return result;
}



bool trace(const point3& e, const point3& s, colour3& colour, bool pick, int recursionLevel, bool outside) {
	colour = background_colour;
	float dist = FLT_MAX;
	point3 D = (s - e);
	int indexOfClosest = -1;
	int indexOfTriangle = -1;
	
	bool hit = getIntersection(e, D, dist, indexOfClosest, indexOfTriangle);

	if (hit) {
		colour3 total = colour3(0, 0, 0);
		Object *object = objects[indexOfClosest];

		point3 P = e + (dist * D); // Point of intersection
		point3 N = calcNormal(object, P, indexOfTriangle); // Normal at intersection point
		point3 V = glm::normalize(e - P); // Vector from P to eye.
		
		for (int i = 0; i < lights.size(); i++) { // For each light
			Light *light = lights[i];
			point3 L = point3(0, 0, 0);
			point3 lightPos = point3(0, 0, 0);

			if (!determineLightDirection(P, light, L, lightPos)) { continue; }
			if (checkIfInShadow(P, light, lightPos, indexOfClosest, indexOfTriangle)) { continue; }
			total += phongIllumination(object, light, N, L, V);
		}

		debugPrintHit(object, indexOfClosest, N, P, pick);
		debugBreakpoints(P, s, V, recursionLevel);

		total += calcReflection(object, P, N, V, outside, pick, recursionLevel);
		total = calcTransmission(object, P, V, total, outside, pick, recursionLevel);
		total = calcRefraction(object, P, N, V, total, outside, pick, recursionLevel);

		colour = total;
	}

	printPickingInfo(hit, pick, recursionLevel, dist, indexOfClosest, indexOfTriangle, colour);

	return hit;
}

