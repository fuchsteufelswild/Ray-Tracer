#pragma once

#include "AccelerationStructure.h"

namespace actracer
{

class Primitive;
class AccelerationStructure;

class AccelerationStructureFactory
{
public:
    static AccelerationStructure *CreateAccelerationStructure(AccelerationStructure::AccelerationStructureAlgorithmCode algorithmCode, 
                                                              const std::vector<Primitive *> &primitives);
};

}