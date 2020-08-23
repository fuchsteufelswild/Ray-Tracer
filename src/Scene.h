#ifndef _SCENE_H_
#define _SCENE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "acmath.h"
#include "Shape.h"
#include "Random.h"

#include <unordered_map>

namespace actracer {

class Camera;
class Light;
class Material;
// class Shape;
class Image;
class Primitive;
class BVHTree;
class TMO;
class BRDFBase;
union Color;


class Scene {
private:
    TMO* tmo;
    Material* defMat; // Default material ( air )

    Texture* bgTexture;
public:
    static Scene* pScene;

    static int debugBegin;
    static int debugEnd;
    static int debugCurrent;

public:
    unsigned seed;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution;

    Random<double> sceneRandom;

    std::vector<Vector3f> inVector;

    int maxRecursionDepth; // Maximum recursion depth
    float intTestEps;          // IntersectionTestEpsilon
    float shadowRayEps;        // ShadowRayEpsilon
    Vector3f backgroundColor;  // Background color
    Vector3f ambientLight;     // Ambient light radiance

    std::vector<Camera *> cameras;      // All cameras in the scene
    std::vector<Light *> lights;        // All lights in the scene
    std::vector<Material *> materials;  // All materials in the scene
    std::vector<Vector3f> vertices;     // All vertex points in the scene
    std::vector<Vector2f> vertexCoords; // All vertex coords
    std::vector<Shape *> objects;       // All shapes in the scene
    std::vector<Vertex*> tVertices;     // All triangle vertices in the scene
    std::vector<Primitive*> primitives; // All primitives in the scene
    std::vector<BRDFBase*> brdfs; // all brdfs
    
    std::vector<Scaling*> scalings;
    std::vector<Rotation *> rotations;
    std::vector<Translation *> translations;
    std::vector<Transformation*> transformations; // All transformations in the scene

    std::vector<std::string> imagePaths;
    std::vector<Texture *> textures;

    std::unordered_map<int, Ray> meshRays;

    BVHTree* accelerator;

    Scene(const char *xmlPath); // Constructor. Parses XML file and initializes vectors above

    void RenderScene(void); // Method to render scene, an image is created for each camera in the scene
private:
    void RenderCam(Camera *cam, Image &img, int iStart, int iEnd);     // Render the scene for camera
    Color RenderPixel(Ray &ray, float ctw = 0.0f, float rth = 0.0f);   // Calculate the color of a pixel
    void CalculateLight(Ray &r, Vector3f &outColor, int depth, float columnToWidth = 0.0f, float rowToHeight = 0.0f); // Compute the contribution of the all lights for the given ray

    Color RenderPixel(Camera* cam, int row, int col);

    void ComputeTransformMatrix(const char* str, Transform& tr);
    Shape *GetMeshWithID(int id, Shape::ShapeType shapeType);

    void ComputeTiltedGlossyReflectionRay(Vector3f &vrd, const Material* intersectionMat); // Get glossy
};

}



#endif
