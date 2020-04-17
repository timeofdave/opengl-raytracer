#version 150

// --------------------- Constants
const int NUM_OBJECTS = 20;
const int NUM_LIGHTS = 10;
const int SPACE_GEOMETRY = 1000;
const int SPACE_MATERIALS = 100;
const int RECURSION_LIMIT = 20; // 1 is just the primary ray
const int TRIANGLES_LIMIT = 20; // Per mesh
const int NUM_SHADOW_RAY = 50;
const float FLT_MAX = 16000000; // Probably not the best value
const float ANTI_ACNE = 0.001f;
const vec3 ZEROS = vec3(0, 0, 0);
const vec3 ONES = vec3(1, 1, 1);
const float FAKE_FOV = 0.6;
const float WORTH_RECURSING = 0.1;

// --------------------- Function Headers
bool trace();
void debugRed();
void debugViewRect(float xPos, float yPos);
bool getIntersection(vec3 e, vec3 d, inout float dist, inout int indexOfClosest, inout int indexOfTriangle);
bool testIntersectionWithObject(int i, vec3 e, vec3 d, inout float dist, inout int indexOfClosest, inout int indexOfTriangle);
vec3 getShadowAmount(vec3 P, int lid, vec3 lightPos);
bool determineLightDirection(vec3 P, int lid, inout vec3 L, inout vec3 lightPos);
vec3 phongIllumination(int oid, int lid, vec3 N, vec3 L, vec3 V);
vec3 calcNormal(int oid, vec3 P, int indexOfTriangle);
vec3 calcReflection(int oid, vec3 P, vec3 N, vec3 V);
vec3 calcTransmission(int oid, vec3 P, vec3 N, vec3 V);
vec3 calcRefraction(int oid, vec3 P, vec3 N, vec3 V);
float calcPlaneDistance(vec3 A, vec3 N, vec3 d, vec3 e);
float acneThreshold(vec3 N, vec3 d);
void initNewRay(vec3 e, vec3 D, bool outside, vec3 effectiveness);
vec3 sumOutputColour();
void clamp(inout vec3 vec);
mat4 rotationMatrix(vec3 axis, float angle);

// --------------------- Structs
struct RayResult {
  vec3 colour;
  vec3 e;
  vec3 D;
  vec3 effectiveness;
  bool outside;
};

// --------------------- Uniforms
out vec4 out_colour;
uniform vec2 Window;
uniform vec3 eyePos;
uniform int objectIds[NUM_OBJECTS];
uniform int lightIds[NUM_LIGHTS];
uniform vec3 geometry[SPACE_GEOMETRY];
uniform vec3 materials[SPACE_MATERIALS];
uniform int numObjects;
uniform int numLights;

// --------------------- Debug Variables
float debug = 1234567.0; // flag value, means not debugging
float debugMin = 0.0;  // black
float debugMax = 1.0; // red

// --------------------- General Variables
RayResult rays[RECURSION_LIMIT];
int currRay = 0;
int numRays = 1;
vec3 background = vec3(0, 0, 0);


void main() { 
    out_colour.a = 1;
    float aspectRatio = Window.x / Window.y;

    float yPos = (gl_FragCoord.y / Window.y) * 2 - 1;
    float xPos = (gl_FragCoord.x / Window.x) * aspectRatio * 2 - (aspectRatio);

    vec3 e = eyePos;
    vec3 s = vec3(e.x + xPos * FAKE_FOV, e.y + yPos * FAKE_FOV, e.z - 1.0);

	rays[0].e = e;
	rays[0].D = s - e;
	rays[0].effectiveness = vec3(1, 1, 1);
	rays[0].outside = true;

	for (currRay = 0; currRay < RECURSION_LIMIT; currRay++) {
		if (currRay >= numRays) { break; }

        trace();
    }

	out_colour.rgb = sumOutputColour();

    debugRed();
}


bool trace() {
	vec3 e = rays[currRay].e;
	vec3 D = rays[currRay].D;

	vec3 total = vec3(0, 0, 0);
	float dist = FLT_MAX;
	int indexOfClosest = -1;
	int indexOfTriangle = -1;
	bool hit = false;

	if (rays[currRay].outside) {
		hit = getIntersection(e, D, dist, indexOfClosest, indexOfTriangle);
	}
	else {
		hit = getIntersection(e, D, dist, indexOfClosest, indexOfTriangle);
	}

	if (hit) {
		int oid = objectIds[indexOfClosest];

		int matid = int(geometry[oid + 1].r);

		vec3 P = e + (dist * D); // Point of intersection
		vec3 N = calcNormal(oid, P, indexOfTriangle); // Normal at intersection point
		vec3 V = normalize(e - P); // Vector from P to eye.
		
		if (rays[currRay].outside) { // Don't bother lighting inside an object.
			for (int i = 0; i < NUM_LIGHTS; i++) { // For each light
				if (i == numLights) { break; }
				int lid = lightIds[i];

				vec3 L = vec3(0, 0, 0);
				vec3 lightPos = vec3(0, 0, 0);

				if (!determineLightDirection(P, lid, L, lightPos)) { continue; }

				vec3 throughLight = getShadowAmount(P, lid, lightPos);
				bool inShadow = (length(throughLight) < 0.1);

				if (!inShadow) {
					total += phongIllumination(oid, lid, N, L, V) * throughLight;
				}
			}
		}
		
		vec3 reflection = calcReflection(oid, P, N, V);
		vec3 transmission = calcTransmission(oid, P, N, V);
		vec3 refraction = calcRefraction(oid, P, N, V);
		
		rays[currRay].colour = total;
		rays[currRay].effectiveness = (1 - (transmission + refraction)) * rays[currRay].effectiveness;
		
	} // if hit
	else {
		rays[currRay].colour = background;
		rays[currRay].effectiveness = ZEROS;
	}
    
    return (indexOfClosest != -1);
}


vec3 sumOutputColour() {
	vec3 outputColour = ZEROS;

	for (int i = 0; i < RECURSION_LIMIT; i++) {
		if (i >= numRays) { break; }

		outputColour += rays[i].colour * rays[i].effectiveness;
	}
	return outputColour;
}


bool getIntersection(vec3 e, vec3 d, inout float dist, inout int indexOfClosest, inout int indexOfTriangle) {

	for (int i = 0; i < NUM_OBJECTS; i++) {
		if (i == numObjects) { break; }
		
        testIntersectionWithObject(i, e, d, dist, indexOfClosest, indexOfTriangle);
	}

	return (indexOfClosest >= 0);
}

bool testIntersectionWithObject(int i, vec3 e, vec3 d, inout float dist, inout int indexOfClosest, inout int indexOfTriangle) {
	int oid = objectIds[i];

	if (geometry[oid].r == 0) { // if sphere
		vec3 pos = geometry[oid + 2];
		vec3 emc = e - pos;
		float r = float(geometry[oid].b);

		float discriminant = dot(d, emc) * dot(d, emc) - dot(d, d) * (dot(emc, emc) - r * r);
		float t = FLT_MAX;

		if (discriminant >= 0.f) {
			// One or two intersections, I don't think I care which.
			float t1 = (dot(-d, emc) + sqrt(discriminant)) / dot(d, d);
			float t2 = (dot(-d, emc) - sqrt(discriminant)) / dot(d, d);
			t1 = (t1 > ANTI_ACNE) ? t1 : FLT_MAX;
			t2 = (t2 > ANTI_ACNE) ? t2 : FLT_MAX;
			t = min(t1, t2);

			if (t < dist) {
				dist = t;
				indexOfClosest = i;
				return true;
			}
		}
	}
	else if (geometry[oid].r == 1) {
		vec3 A = geometry[oid + 2]; // object->pos
		vec3 N = geometry[oid + 3]; // object->normal

		float t = calcPlaneDistance(A, N, d, e);

		if (t > acneThreshold(N, d)) { // hit the plane
			if (t < dist) {
				dist = t;
				indexOfClosest = i;
				return true;
			}
		}
	}
	else if (geometry[oid].r == 2) {
		int numTris = int(geometry[oid].g);

		for (int j = 0; j < TRIANGLES_LIMIT; j++) {
			if (j == numTris) { break; }

			int tid = oid + 4 + (j * 4); // Triange id

			vec3 A = geometry[tid + 0];
			vec3 B = geometry[tid + 1];
			vec3 C = geometry[tid + 2];
			vec3 N = geometry[tid + 3];

			float t = calcPlaneDistance(A, N, d, e);
			vec3 X = e + (t * d);

			float inA = dot(cross(B - A, X - A), N);
			float inB = dot(cross(C - B, X - B), N);
			float inC = dot(cross(A - C, X - C), N);

			if (t > acneThreshold(N, d) && inA > 0.f && inB > 0.f && inC > 0.f) { // hit front of triangle
				if (t < dist) {
					dist = t;
					indexOfClosest = i;
					indexOfTriangle = j;
					return true;
				}
			}
		} // for each triangle
	}
	return false;
}



// ------------------- SHADOW CHECK -----------------------
// get the amount of light that makes it to this point
// Cast a ray from the point P to the light.
vec3 getShadowAmount(vec3 P, int lid, vec3 lightPos) {
    int lightType = int(geometry[lid].r);
	vec3 throughLight = vec3(1,1,1); // default all light makes it through
	int hitCount = 0;
	if (lightType > 3) { // AMBIENT
		
		vec3 shadowRay = normalize(lightPos - P);
		float areaRadius = float(geometry[lid].g);

		if(areaRadius <= 0) {
			for (int i = 0; i < NUM_OBJECTS; i++) {

				if (i == numObjects) // this is needed otherwise this breaks
					break;

				int oid = objectIds[i];
				float maxDist = length(lightPos - P);
				int indexObjL = -1;
				int indexTriL = -1;
				
				// test every object for intersection
				if(testIntersectionWithObject(i, P, shadowRay, maxDist, indexObjL, indexTriL)) {
					int matid = int(geometry[oid + 1].r);
					vec3 transmission = materials[matid + 4];
					throughLight = throughLight * transmission;
				}
			} // for each object
		} else {

			vec3 totalLight = vec3(0,0,0);
			int numRays = int(NUM_SHADOW_RAY * (areaRadius/0.5f));
			
			for(int j = 0; j < NUM_SHADOW_RAY; j++) {

				if (j == numRays)
					break;

				vec3 lightPortion = vec3(1,1,1) * (1.0f/numRays);

				vec3 perpVector = normalize(cross(lightPos - P, vec3(1,1,1))) * areaRadius;
				float radians = radians(360.0f/NUM_SHADOW_RAY * j);
				mat4 rotation = rotationMatrix(lightPos - P, radians);
				vec3 areaScale = perpVector;
				vec4 areaScale4;
				areaScale4.xyz = areaScale.xyz;
				areaScale4.w = 0;		
				
				vec3 lightSpot = lightPos + (rotation * areaScale4).xyz;
				vec3 shadowRay = normalize(lightSpot - P);			
				
				for (int i = 0; i < NUM_OBJECTS; i++) {
					if (i == numObjects) // this is needed otherwise this breaks
						break;

					int oid = objectIds[i];
					float maxDist = length(lightPos - P);
					int indexObjL = -1;
					int indexTriL = -1;
					
					// test every object for intersection
					if(testIntersectionWithObject(i, P, shadowRay, maxDist, indexObjL, indexTriL)) {
						int matid = int(geometry[oid + 1].r);
						vec3 transmission = materials[matid + 4];
						lightPortion = lightPortion * transmission;
					}
				}

				totalLight += lightPortion;
			}

			throughLight = totalLight;
	
				
		}

	}
	return throughLight;
}

bool determineLightDirection(vec3 P, int lid, inout vec3 L, inout vec3 lightPos) {
	bool isVisible = true;
    int type = int(geometry[lid].r);

	if (type == 4) { // DIRECTIONAL
        vec3 direction = geometry[lid + 3];
		L = normalize(-1.0f * direction);
		lightPos = P + (100.f * L); // Has to be further away than any object.
	}
	else if (type == 5) { //POINT_LIGHT
		lightPos = geometry[lid + 2];
		L = normalize(lightPos - P);
	}
	else if (type == 6) { // SPOT
		lightPos = geometry[lid + 2];
        vec3 direction = geometry[lid + 3];

		L = normalize(lightPos - P);
		vec3 lightFacing = normalize(direction);
		float cutoff = radians(float(geometry[lid].b));
		float dotProduct = dot(L, -lightFacing);

		if (dotProduct < cos(cutoff)) {
			isVisible = false; // Just pretend this light doesn't exist.
		}
	}
	return isVisible;
}


// Calculate lighting equation at the hit point.
vec3 phongIllumination(int oid, int lid, vec3 N, vec3 L, vec3 V) {
	vec3 total = ZEROS;
    int matid = int(geometry[oid + 1].r);
    vec3 ambient = materials[matid + 0];
    vec3 diffuse = materials[matid + 1];
    vec3 specular = materials[matid + 2];
    float shininess = materials[matid + 5].r;
    int lightType = int(geometry[lid].r);
    vec3 lightColour = geometry[lid + 1];

	// Ambient component
	if (ambient != ZEROS && lightType == 3) { // AMBIENT
		total += lightColour * ambient;
	}

	// Diffuse component
	if (diffuse != ZEROS && lightType > 3) {
		float dotProduct = dot(N, L);
		total += lightColour * diffuse * max(0.f, dotProduct);
	}

	// Specular component
	if (specular != ZEROS && lightType > 3) {
		vec3 R = normalize(2.f * dot(N, L) * N - L); // Reflection direction
		float dotProduct = dot(R, V);

		if (dotProduct > 0) {
			float shine = pow(dotProduct, shininess);
			total += lightColour * specular * shine;
		}
	}

	return total;
}

void initNewRay(vec3 e, vec3 D, bool outside, vec3 effectiveness) {
	rays[numRays].e = e;
	rays[numRays].D = D;
	rays[numRays].outside = outside;
	rays[numRays].effectiveness = effectiveness;
	numRays++;
}


vec3 calcReflection(int oid, vec3 P, vec3 N, vec3 V) {
	int matid = int(geometry[oid + 1].r);
	vec3 reflective = materials[matid + 3];
	vec3 reflectionEffectiveness = reflective * rays[currRay].effectiveness;

	if (reflective != ZEROS && numRays < RECURSION_LIMIT && rays[currRay].outside
		&& length(reflectionEffectiveness) > WORTH_RECURSING) {

		vec3 R = normalize(2.0 * dot(N, V) * N - V); // Reflection direction
		
		initNewRay(P, R, rays[currRay].outside, reflectionEffectiveness);
	}
	else {
		reflective = ZEROS;
	}

	return reflective;
}


vec3 calcTransmission(int oid, vec3 P, vec3 N, vec3 V) {
	int matid = int(geometry[oid + 1].r);
	vec3 transmissive = materials[matid + 4];
	float refraction = materials[matid + 5].g;
	vec3 transmissionEffectiveness = transmissive * rays[currRay].effectiveness;

	if (transmissive != ZEROS && numRays < RECURSION_LIMIT && refraction == 0.0
		&& length(transmissionEffectiveness) > WORTH_RECURSING) {

		bool outside = rays[currRay].outside;
		bool goingOutside = (geometry[oid].r == 1) ? outside : !outside; // If not plane, flip it.
		
		initNewRay(P, -V, goingOutside, transmissionEffectiveness);
	}
	else {
		transmissive = ZEROS;
	}

	return transmissive;
}


vec3 calcRefraction(int oid, vec3 P, vec3 N, vec3 V) {
	int matid = int(geometry[oid + 1].r);
	vec3 transmissive = materials[matid + 4];
	float refraction = materials[matid + 5].g;
	vec3 refractionEffectiveness = transmissive * rays[currRay].effectiveness;

	if (transmissive != ZEROS && numRays < RECURSION_LIMIT && refraction != 0.0
		&& length(refractionEffectiveness) > WORTH_RECURSING) {
		
		bool outside = rays[currRay].outside;
		bool goingOutside = (geometry[oid].r == 1) ? outside : !outside; // If not plane, flip it.

		vec3 vEye = -V;
		vec3 norm = (outside) ? N : -N;

		float indexInc = (outside) ? 1.f : refraction;
		float indexRef = (goingOutside) ? 1.f : refraction;

		float numeratorPart2 = (1 - pow(dot(vEye, norm), 2));
		float indicesRatio = (indexInc * indexInc) / (indexRef * indexRef);
		float insideSqrt = 1 - indicesRatio * numeratorPart2;

		if (insideSqrt < 0) { // Total Internal Reflection
			vec3 R = normalize(2.0 * dot(norm, V) * norm - V);
			
			initNewRay(P, R, goingOutside, refractionEffectiveness);
		}
		else { // Refraction
			vec3 numerator1 = indexInc * (vEye - norm * dot(vEye, norm));
			vec3 R = normalize((numerator1 / indexRef) - norm * sqrt(insideSqrt));

			initNewRay(P, R, goingOutside, refractionEffectiveness);
		}
	}
	else {
		transmissive = ZEROS;
	}

	return transmissive;
}


vec3 calcNormal(int oid, vec3 P, int indexOfTriangle) {
	vec3 N = ZEROS;
    int type = int(geometry[oid].r);
	
	if (type == 0) {
		vec3 center = geometry[oid + 2];
		N = normalize(P - center);
	}
	if (type == 1) {
        vec3 normal = geometry[oid + 3];
		N = normalize(normal);
	}
	if (type == 2) {
        int tid = oid + 4 + (indexOfTriangle * 4); // Triange id
		N = normalize(geometry[tid + 3]);
	}
	return N;
}


float calcPlaneDistance(vec3 A, vec3 N, vec3 d, vec3 e) {
	float denom = dot(N, d);
	float t = 0.f;

	if (denom > 0) {
		t = dot(N, A - e) / denom;
		if (t > 0) {
			t = t;
		}
	}
	if (denom < 0) {
		t = dot(N, A - e) / denom;
	}
	return t;
}

float acneThreshold(vec3 N, vec3 d) {
	float acneThreshold = ANTI_ACNE;
	float angleOfIncidenceCos = dot(N, d);

	if (angleOfIncidenceCos > 0) {
		acneThreshold = ANTI_ACNE / angleOfIncidenceCos;
	}

	return acneThreshold;
}

void clamp(inout vec3 vec) {
	vec.r = max(0, min(1, vec.r));
	vec.g = max(0, min(1, vec.g));
	vec.b = max(0, min(1, vec.b));
}


void debugRed() {
    if (debug != 1234567.0) {
        float red = (debug - debugMin) / (debugMax - debugMin);
        out_colour.r = red;
        out_colour.g = 0;
        out_colour.b = 0;
        out_colour.a = 1;
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

//http://www.neilmendoza.com/glsl-rotation-about-an-arbitrary-axis/
mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}