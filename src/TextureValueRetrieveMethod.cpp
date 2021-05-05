#include "TextureValueRetrieveMethod.h"
#include "TextureReader.h"

namespace actracer
{

TextureValueRetrieveMethod* TextureValueRetrieveMethod::CreateTextureValueRetrieveMethod(InterpolationMethodCode interpolationMethodCode)
{
    switch(interpolationMethodCode)
    {
        case InterpolationMethodCode::NEAREST:
            return new NearestTextureValueRetrieveMethod();
        case InterpolationMethodCode::BILINEAR:
            return new BilinearTextureValueRetrieveMethod();
    }

    return nullptr;
}

Vector3f TextureValueRetrieveMethod::RetrieveValueFromUVCoordinate(const TextureReader *reader, float u, float v) const
{       
    std::pair<float, float> rawPixelCoordinates = GetRawRowColumnPositions(u, v, reader->GetWidth(), reader->GetHeight());

    return RetrieveValueFromPixelCoordinate(reader, rawPixelCoordinates.second, rawPixelCoordinates.first);
}

std::pair<float, float> TextureValueRetrieveMethod::GetRawRowColumnPositions(float u, float v, int width, int height) const
{
    if (u > 1.0f)
        u = u - (int)u;
    if (v > 1.0f)
        v = v - (int)v;
        
    return std::make_pair<float, float>(u * (width - 1), v * (height - 1));
}

Vector3f NearestTextureValueRetrieveMethod::RetrieveValueFromPixelCoordinate(const TextureReader *reader, float rawRowPosition, float rawColumnPosition) const
{
    int row = round(rawRowPosition);
    int column = round(rawColumnPosition);
    return reader->FetchPixelValueFromTexture(row, column);
}

Vector3f BilinearTextureValueRetrieveMethod::RetrieveValueFromPixelCoordinate(const TextureReader *reader, float rawRowPosition, float rawColumnPosition) const
{
    int row = floor(rawRowPosition);
    int column = floor(rawColumnPosition);
    float rowOffsetFromCenter = rawRowPosition - row;
    float columnOffsetFromCenter = rawColumnPosition - column;

    return reader->FetchPixelValueFromTexture(row, column) * (1 - rowOffsetFromCenter) * (1 - columnOffsetFromCenter) +
           reader->FetchPixelValueFromTexture(row + 1, column) * (rowOffsetFromCenter) * (1 - columnOffsetFromCenter) +
           reader->FetchPixelValueFromTexture(row, column + 1) * (1 - rowOffsetFromCenter) * (columnOffsetFromCenter) +
           reader->FetchPixelValueFromTexture(row + 1, column + 1) * (rowOffsetFromCenter) * (columnOffsetFromCenter);
}

}