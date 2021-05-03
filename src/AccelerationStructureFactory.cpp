#include "AccelerationStructureFactory.h"
#include "BVHTree.h"

#include <vector>

namespace actracer
{
    AccelerationStructure *AccelerationStructureFactory::CreateAccelerationStructure(AccelerationStructure::AccelerationStructureAlgorithmCode algorithmCode,
                                                                                     const std::vector<Primitive *> &primitives)
    {

        switch (algorithmCode)
        {
        case AccelerationStructure::AccelerationStructureAlgorithmCode::BVH:
            return new BVHTree(1, 10000, primitives);
        default:
            break;
        }

        return nullptr;
    }
}
