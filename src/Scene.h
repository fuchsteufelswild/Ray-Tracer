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

class RenderStrategy;
class Camera;
class Light;
class Material;
// class Shape;
class Image;
class Primitive;
class BVHTree;
class Tonemapper;
class BRDFBase;
union Color;

class Scene {
friend class SceneParser;

private:
    RenderStrategy* mRenderStrategy;

private:
    Tonemapper *tmo;
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

    Scene();
    Scene(const char *xmlPath); // Constructor. Parses XML file and initializes vectors above

    void RenderScene(void); // Method to render scene, an image is created for each camera in the scene
public:
    const std::vector<Camera*>& GetAllCameras() const;
    const std::vector<Primitive*>& GetAllPrimitives() const;
    const std::vector<Light*>& GetAllLights() const;
    
    const Tonemapper* GetTonemapper() const;
    const Texture* GetBackgroundTexture() const;

    int GetMaximumRecursionDepth() const;
    float GetIntersectionTestEpsilon() const;
    float GetShadowRayEpsilon() const;
    Vector3f GetBackgroundColor() const;
    Vector3f GetAmbientColor() const;

private:
    void RenderCam(Camera *cam, Image &img, int iStart, int iEnd);     // Render the scene for camera
    Color RenderPixel(Ray &ray, float ctw = 0.0f, float rth = 0.0f);   // Calculate the color of a pixel
    void CalculateLight(Ray &r, Vector3f &outColor, int depth, float columnToWidth = 0.0f, float rowToHeight = 0.0f); // Compute the contribution of the all lights for the given ray

    Color RenderPixel(Camera* cam, int row, int col);

    void ComputeTransformMatrix(const char* str, Transform& tr);
    Shape *GetMeshWithID(int id, Shape::ShapeType shapeType);

    void ComputeTiltedGlossyReflectionRay(Vector3f &vrd, const Material* intersectionMat); // Get glossy
};

inline const Tonemapper *Scene::GetTonemapper() const
{
    return tmo;
}

inline const Texture *Scene::GetBackgroundTexture() const
{
    return bgTexture;
}

inline const std::vector<Light *> &Scene::GetAllLights() const
{
    return lights;
}

inline const std::vector<Camera *> &Scene::GetAllCameras() const
{
    return cameras;
}

inline const std::vector<Primitive *> &Scene::GetAllPrimitives() const
{
    return primitives;
}

inline int Scene::GetMaximumRecursionDepth() const
{
    return maxRecursionDepth;
}

inline float Scene::GetIntersectionTestEpsilon() const
{
    return intTestEps;
}

inline float Scene::GetShadowRayEpsilon() const
{
    return shadowRayEps;
}

inline Vector3f Scene::GetBackgroundColor() const
{
    return backgroundColor;
}

inline Vector3f Scene::GetAmbientColor() const
{
    return ambientLight;
}

}



#endif
