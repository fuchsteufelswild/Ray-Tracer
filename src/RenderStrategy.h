#pragma once

#include "Random.h"
#include "acmath.h"

namespace actracer
{

class Scene;

class RenderStrategy
{
public:
    virtual void RenderSceneIntoPPM(Scene* scene) = 0;
    
protected:
    virtual void RetrieveRenderingParamsFromScene(Scene *scene);

protected:
    Random<double> randomGenerator;
};

} 
