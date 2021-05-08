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
    
	Scene* currentScene = currentScene = SceneParser::CreateSceneFromXML(xmlPath);
    RenderStrategy* renderer = new DefaultRenderer();
	std::cout << "Scene is parsed\n";
	system("pause");
    renderer->RenderSceneIntoPPM(currentScene);
    system("pause");

	delete renderer;
	delete currentScene;

    return 0;
}