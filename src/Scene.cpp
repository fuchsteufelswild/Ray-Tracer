#include "Light.h"
#include "Material.h"
#include "Camera.h"
#include "Image.h"
#include "Scene.h"

// #include "Shape.h"
#include "Sphere.h"
#include "Triangle.h"
#include "Mesh.h"
#include "Primitive.h"
#include "BVHTree.h"
#include "tinyxml2.h"
#include "happly.h"
#include "Texture.h"
#include "Tonemapper.h"
#include "brdf.h"
// #include <future>
#include <thread>
#include <string>
#include <chrono>
#include <memory>
#include <cmath>

using namespace tinyxml2;

namespace actracer {

int Primitive::id = 0;

Scene::Scene()
{
    tmo = nullptr;

    sceneRandom = Random<double>{};

    maxRecursionDepth = 0; // Default recursion depth
    shadowRayEps = 0.005;  // Default shadow ray epsilon
    intTestEps = 0.0001;
}

Shape* Scene::GetMeshWithID(int id)
{
    for(Shape* sh : objects)
    {
        if(id == sh->GetID() && dynamic_cast<Mesh*>(sh) != nullptr)
        {
            return sh;
        }
    }
}


}
