#include <iostream>
#include "acmath.h"
#include "Scene.h"
#include "Primitive.h"
#include "AccelerationStructureFactory.h"
#include "Timer.h"
#include <chrono>

#include "SceneParser.h"

#include "DefaultRenderer.h"

using namespace actracer;

int main(int argc, char* argv[])
{
    const char *xmlPath = argv[1];
    Scene* currentScene;
    BVHTree *bvh;

    {
        currentScene = SceneParser::CreateSceneFromXML(xmlPath);

        std::cout << "Before parse\n";
        // currentScene = new Scene(xmlPath);
        std::cout << "After parse\n";
            Timer t("Tree Building");
        // bvh = new BVHTree(1, 10000, currentScene->primitives);  
    }
    
    RenderStrategy* renderer = new DefaultRenderer();
    

    // currentScene->accelerator = bvh;
    system("pause");
    renderer->RenderSceneIntoPPM(currentScene);
    // currentScene->RenderScene();
    system("pause");

    return 0;
}