#include <iostream>
#include "acmath.h"
#include "Scene.h"
#include "Primitive.h"
#include "Acceleration.h"
#include "Timer.h"
#include <chrono>

using namespace actracer;

int main(int argc, char* argv[])
{
    const char *xmlPath = argv[1];
    Scene* currentScene;
    BVHTree *bvh;

    {
        std::cout << "Before parse\n";
        currentScene = new Scene(xmlPath);
        std::cout << "After parse\n";
            Timer t("Tree Building");
        bvh = new BVHTree(1, 10000, currentScene->primitives);  
    }
    currentScene->accelerator = bvh;
    system("pause");
    currentScene->RenderScene();
    system("pause");

    return 0;
}