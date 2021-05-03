#pragma once

#include "Random.h"
#include "acmath.h"

namespace actracer
{

class Scene;
class AccelerationStructure;

class RenderStrategy
{
public:
    virtual void RenderSceneIntoPPM(Scene* scene) = 0;
    RenderStrategy(const AccelerationStructure* accelerator);
protected:
    virtual void RetrieveRenderingParamsFromScene(Scene *scene);


    RenderStrategy() {}
protected:
    Random<double> randomGenerator;
    
};

} 
