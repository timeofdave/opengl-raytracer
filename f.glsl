#version 150

// --------------------- Constants
const int NUM_OBJECTS = 20;
const int NUM_LIGHTS = 10;
const int SPACE_GEOMETRY = 1000;
const int SPACE_MATERIALS = 100;
const int RECURSION_LIMIT = 5;
const int TRIANGLES_LIMIT = 20; // Per mesh
const float FLT_MAX = 16000000; // Probably not the best value
const float EPSILON = 0.0001f;
const float ANTI_ACNE = 0.001f;
const vec3 ZEROS = vec3(0, 0, 0);
const float FAKE_FOV = 0.6;

// --------------------- Function Headers
bool trace(vec3 e, vec3 s, inout vec3 colour);
void debugRed();
void debugViewRect(float xPos, float yPos);
bool getIntersection(vec3 e, vec3 d, inout float dist, out int indexOfClosest, out int indexOfTriangle);
bool checkIfInShadow(vec3 P, int lid, vec3 lightPos, int indexOfClosest, int indexOfTriangle);
bool determineLightDirection(vec3 P, int lid, inout vec3 L, inout vec3 lightPos);
vec3 phongIllumination(int oid, int lid, vec3 N, vec3 L, vec3 V);
vec3 calcNormal(int oid, vec3 P, int indexOfTriangle);
float calcPlaneDistance(vec3 A, vec3 N, vec3 d, vec3 e);
float acneThreshold(vec3 N, vec3 d);

// --------------------- Structs
struct RayResult {
  vec3 colour;
};

// --------------------- Uniforms
out vec4 out_colour;
uniform vec2 Window;
uniform vec3 eyePos;
uniform int objectIds[NUM_OBJECTS];
uniform int lightIds[NUM_LIGHTS];
uniform vec3 geometry[SPACE_GEOMETRY];
uniform vec3 materials[SPACE_MATERIALS];

// --------------------- Debug Variables
float debug = 1234567.0; // flag value, means not debugging
float debugMin = -10.0;
float debugMax = 10.0; // set these to a reasonable range

// --------------------- General Variables
RayResult rays[RECURSION_LIMIT];
int currRay = 0;


void main() { 
    out_colour.a = 1;
    float aspectRatio = Window.x / Window.y;

    float yPos = (gl_FragCoord.y / Window.y) * 2 - 1;
    float xPos = (gl_FragCoord.x / Window.x) * aspectRatio * 2 - (aspectRatio);

    vec3 e = eyePos; //vec3(0, 0, 0);
    vec3 s = vec3(e.x + xPos * FAKE_FOV, e.y + yPos * FAKE_FOV, e.z - 1.0);

    for (currRay = 0; currRay < RECURSION_LIMIT; currRay++) {
        RayResult ray = rays[currRay];
        //vec3 colour = vec3(1, 0, 0);
        if (trace(e, s, ray.colour)) {
            out_colour.rgb = ray.colour;
        }
    }

    //rays[0].colour.z = geometry[2].z;
    //debug = rays[0].colour.z;

    //debugViewRect(xPos, yPos);
    debugRed();
}


bool trace(vec3 e, vec3 s, inout vec3 colour) {
    float dist = FLT_MAX;
    vec3 D = (s - e);
    int indexOfClosest = -1;
	int indexOfTriangle = -1;

    bool hit = getIntersection(e, D, dist, indexOfClosest, indexOfTriangle);
    
    if (hit) {
		vec3 total = vec3(0, 0, 0);
        int oid = objectIds[indexOfClosest];

		vec3 P = e + (dist * D); // Point of intersection
		vec3 N = calcNormal(oid, P, indexOfTriangle); // Normal at intersection point
		vec3 V = normalize(e - P); // Vector from P to eye.
		
		for (int i = 0; i < NUM_LIGHTS; i++) { // For each light
			int lid = lightIds[i];

			vec3 L = vec3(0, 0, 0);
			vec3 lightPos = vec3(0, 0, 0);

			if (!determineLightDirection(P, lid, L, lightPos)) { continue; }
			if (checkIfInShadow(P, lid, lightPos, indexOfClosest, indexOfTriangle)) { continue; }
			total += phongIllumination(oid, lid, N, L, V);
		}

		//total += calcReflection(object, P, N, V, outside, pick, recursionLevel);
		//total = calcTransmission(object, P, V, total, outside, pick, recursionLevel);
		//total = calcRefraction(object, P, N, V, total, outside, pick, recursionLevel);

		colour = total;
	}
    
    //debug = dist;

    return hit;
}




bool getIntersection(vec3 e, vec3 d, inout float dist, out int indexOfClosest, out int indexOfTriangle) {

	for (int i = 0; i < NUM_OBJECTS; i++) {
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
				}
			}
		}
		else if (geometry[oid].r == 2) {
            int numTris = int(geometry[oid].g);

			for (int j = 0; j < TRIANGLES_LIMIT; j++) {
                if (j == numTris) {
                    break;
                }
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
bool checkIfInShadow(vec3 P, int lid, vec3 lightPos, int indexOfClosest, int indexOfTriangle) {
	bool isInShadow = false;
    int lightType = int(geometry[lid].r);

	if (lightType > 3) { // AMBIENT
		float distL = FLT_MAX;
		int indexObjL = -1;
		int indexTriL = -1;
		vec3 shadowRay = (P - lightPos) / 100.f;
		bool hit = getIntersection(lightPos, shadowRay, distL, indexObjL, indexTriL);

		if (hit) { // should always hit, but there's always room for floating point error.
			vec3 shadowP = lightPos + (distL * shadowRay); // Point of intersection

			//if (glm::distance(P, shadowP) > 0.001) {
			if (indexObjL != indexOfClosest || indexTriL != indexOfTriangle) {
				isInShadow = true; // Pretend this light doesn't exist.
			}
		}
	}
	return isInShadow;
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