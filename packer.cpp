
#include "Object.h"

const colour3 ZEROS = colour3(0, 0, 0);

extern std::vector<Object *> objects;
extern std::vector<Light *> lights;

extern int objectIds[6];
extern int lightIds[6];
extern point3 geometry[6];
extern point3 materials[6];
extern int numObjects;
extern int numLights;

/****************************************************************************/


void packObjects(int &geoId, int &matId) {
	numObjects = objects.size();

	for (int i = 0; i < objects.size(); i++) {
		Object* object = objects[i];

		objectIds[i] = geoId;
		int index = geoId;

		geometry[index++] = point3(object->type, 0, object->radius);
		geometry[index++] = point3(matId, 0, 0);
		geometry[index++] = object->pos;
		geometry[index++] = object->normal;

		int j = 0;
		for (; j < object->tris.size(); j++) {
			Triangle* triangle = object->tris[j];

			geometry[index++] = triangle->vertices[0];
			geometry[index++] = triangle->vertices[1];
			geometry[index++] = triangle->vertices[2];
			geometry[index++] = triangle->normal;
		}
		geometry[geoId].g = j; // Update number of triangles
		geoId = index; // Set geoId to after the end of this entry

		// Material
		index = matId;

		materials[index++] = object->ambient;
		materials[index++] = object->diffuse;
		materials[index++] = object->specular;
		materials[index++] = object->reflective;
		materials[index++] = object->transmissive;
		materials[index++] = point3(object->shininess, object->refraction, 0);

		matId = index; // Set matId to after the end of this entry
	}
}


void packLights(int& geoId) {
	numLights = lights.size();

	for (int i = 0; i < lights.size(); i++) {
		Light* light = lights[i];

		lightIds[i] = geoId;
		int index = geoId;

		geometry[index++] = point3(light->type + 3, 0, light->cutoff);
		geometry[index++] = light->colour;
		geometry[index++] = light->pos;
		geometry[index++] = light->direction;

		geoId = index; // Set geoId to after the end of this entry
	}
}


void pack_scene() {
	int geoId = 0;
	int matId = 0;

	packObjects(geoId, matId);
	packLights(geoId);
}



