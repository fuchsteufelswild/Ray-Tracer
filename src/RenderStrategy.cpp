#include "RenderStrategy.h"

namespace actracer
{
void RenderStrategy::RetrieveRenderingParamsFromScene(Scene *scene)
{
    randomGenerator = Random<double>{};
}

}