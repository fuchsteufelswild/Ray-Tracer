#pragma once

#include "acmath.h"

namespace actracer
{

enum class InterpolationMethodCode
{
    NEAREST,
    BILINEAR
};

class TextureReader;

class TextureValueRetrieveMethod
{
public:
    static TextureValueRetrieveMethod *CreateTextureValueRetrieveMethod(InterpolationMethodCode interpolationMethodCode);

    Vector3f RetrieveValueFromUVCoordinate(const TextureReader *reader, float u, float v) const;
protected:
    TextureValueRetrieveMethod() { }

    virtual Vector3f RetrieveValueFromPixelCoordinate(const TextureReader* reader, float rawRowPosition, float rawColumnPosition) const = 0;
    std::pair<float, float> GetRawRowColumnPositions(float u, float v, int width, int heigh) const;
};

class NearestTextureValueRetrieveMethod : public TextureValueRetrieveMethod
{
public:
    NearestTextureValueRetrieveMethod() { }

    virtual Vector3f RetrieveValueFromPixelCoordinate(const TextureReader *reader, float rawRowPosition, float rawColumnPosition) const override;
};

class BilinearTextureValueRetrieveMethod : public TextureValueRetrieveMethod
{
public:
    BilinearTextureValueRetrieveMethod() { }

    virtual Vector3f RetrieveValueFromPixelCoordinate(const TextureReader *reader, float rawRowPosition, float rawColumnPosition) const override;
};

}