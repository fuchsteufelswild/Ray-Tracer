#include "Light.h"
#include "Material.h"
#include "Shape.h"
#include "Texture.h"



namespace actracer {

static float deg2radians = 3.1415926f / 180.0f;
static float rad2deg = 180.0f / 3.1415926f;

Light::Light(const Vector3f &position, const Vector3f &intensity)
    : lightPosition(position), lightIntenstiy(intensity) {}

AdjustableLight::AdjustableLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction)
    : Light(position, intensity), lightDirection(direction) {}

// Directional Light
Vector3f DirectionalLight::ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, Texture *tex, float gamma)
{
    float gam = 1.0f;
    if(target.mat->degamma)
        gam = gamma;
    Vector3f texColor{};
    if (tex != nullptr)
    {
        if (tex->texType == Texture::TextureType::IMAGE)
        {
            texColor = tex->RetrieveRGBFromUV(target.uv.x, target.uv.y);
            texColor /= tex->normalizer;
        }
        else
            texColor = tex->RetrieveRGBFromUV(target.ip.x, target.ip.y, target.ip.z);
    }

    Vector3f pointToLight = -Normalize(lightDirection);

    Vector3f outColor(0.0f, 0.0f, 0.0f);

    float dp = Dot(pointToLight, target.n);

    if (dp <= 0) // The are at right angles or light intersects with the back face of the object
        return outColor;

    Vector3f regulatedLightIntensity = lightIntenstiy; // Resulting light intensity at the intersection point

    // Diffuse Reflection

    Vector3f resultingDp = (target.mat->DRC + texColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_KD)
        outColor = outColor + (texColor)*regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else if (tex != nullptr && tex->decalMode == Texture::DecalMode::BLEND_KD)
        outColor = outColor + resultingDp * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else
        outColor = outColor + target.mat->DRC * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));

    Vector3f add = pointToLight + viewerDirection; // Halfway vector
    Vector3f halfwayVector = Normalize(add);

    dp = Dot(halfwayVector, target.n); // Correlation between halfway vector and the normal
    if (dp > 0)
        outColor = outColor + regulatedLightIntensity * static_cast<float>(std::pow(dp, target.mat->phongExponent)) * target.mat->SRC; // Add specular reflection

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_ALL)
        outColor = texColor;
    return outColor;
}
// --

// Spot Light
SpotLight::SpotLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction, float ca, float fa, float exponent)
    : AdjustableLight(position, intensity, direction), spotLightExponent(exponent), coverageAngle(ca), falloffAngle(fa) { lType = LightType::SPOT; }

Vector3f SpotLight::ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, Texture *tex, float gamma)
{
    lightDirection = Normalize(lightDirection);
    Vector3f texColor{};
    if (tex != nullptr)
    {
        if (tex->texType == Texture::TextureType::IMAGE)
        {
            texColor = tex->RetrieveRGBFromUV(target.uv.x, target.uv.y);
            texColor /= tex->normalizer;
        }
        else
            texColor = tex->RetrieveRGBFromUV(target.ip.x, target.ip.y, target.ip.z);
    }

    Vector3f outColor(0.0f, 0.0f, 0.0f);

    Vector3f pointToLight = lightPosition - target.ip; // Light direction

    float distanceToLight = SqLength(pointToLight); // Get the distance

    pointToLight = Normalize(pointToLight); // Normalize for accurate calculation of correlation

    float theta = std::acos(-Dot(pointToLight, lightDirection));

    float falloff = (std::cos(theta) - std::cos(coverageAngle * deg2radians / 2)) / (std::cos(falloffAngle * deg2radians / 2) - std::cos(coverageAngle * deg2radians / 2));

    float dp = Dot(pointToLight, target.n); // Correlation between the light direction and the normal

    if (dp <= 0) // The are at right angles or light intersects with the back face of the object
        return outColor;

    Vector3f regulatedLightIntensity = lightIntenstiy / (distanceToLight * distanceToLight); // Resulting light intensity at the intersection point

    if(theta * rad2deg > coverageAngle / 2)
    {
        return outColor;
    }
    else if(theta * rad2deg > falloffAngle / 2)
    {
        regulatedLightIntensity *= falloff;
    }

    Vector3f resultingDp = (target.mat->DRC + texColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_KD)
        outColor = outColor + (texColor)*regulatedLightIntensity * dp;
    else if (tex != nullptr && tex->decalMode == Texture::DecalMode::BLEND_KD)
        outColor = outColor + resultingDp * regulatedLightIntensity * dp;
    else
        outColor = outColor + target.mat->DRC * regulatedLightIntensity * dp;


    // Specular Reflection
    Vector3f add = pointToLight + viewerDirection; // Halfway vector
    Vector3f halfwayVector = Normalize(add);

    dp = Dot(halfwayVector, target.n); // Correlation between halfway vector and the normal
    if (dp > 0)
        outColor = outColor + regulatedLightIntensity * static_cast<float>(std::pow(dp, target.mat->phongExponent)) * target.mat->SRC; // Add specular reflection

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_ALL)
        outColor = texColor;
    return outColor;
}
// --

// Area Light
AreaLight::AreaLight(const Vector3f& p, const  Vector3f& r, const Vector3f& d, float s)
    : AdjustableLight(p, r, d), size(s) { lType = LightType::AREA; }


bool AreaLight::IsPointInside(const Vector3f& target)
{
    if(target.y - lightPosition.y <= 0.3f && target.y > lightPosition.y)
    {
        if(target.x >= lightPosition.x - size / 2 && target.x <= lightPosition.x + size / 2 && 
           target.z >= lightPosition.z - size / 2 && target.z <= lightPosition.z + size / 2)
           return true;
    }

    return false;
}

Vector3f AreaLight::GetLightDirection(Vector3f)
{
    sampledPoint = GetRandomPointInSquare();

    return sampledPoint;
}

Vector3f AreaLight::GetRandomPointInSquare()
{
    double param1 = randPoint(0.0f, 1.0f);
    double param2 = randPoint(0.0f, 1.0f);

    param1 -= 0.5;
    param2 -= 0.5;

    Vector3f toRight = (float)param1 * size * Vector3f(1.0f, 0.0f, 0.0f);
    Vector3f toUp = (float)param2 * size * Vector3f(0.0f, 0.0f, 1.0f);

    return lightPosition + toUp + toRight;
}

Vector3f AreaLight::ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, Texture *tex, float gamma)
{
    float gam = 1.0f;
    if(target.mat->degamma)
        gam = gamma;
    Vector3f texColor{};
    if (tex != nullptr)
    {
        if (tex->texType == Texture::TextureType::IMAGE)
        {
            texColor = tex->RetrieveRGBFromUV(target.uv.x, target.uv.y);
            texColor /= tex->normalizer;
        }
        else
            texColor = tex->RetrieveRGBFromUV(target.ip.x, target.ip.y, target.ip.z);
    }

    Vector3f outColor(0.0f, 0.0f, 0.0f);

    Vector3f pointToLight = target.ip - sampledPoint; // Light direction

 
    float distanceToLight = SqLength(pointToLight); // Get the distance

    pointToLight = Normalize(pointToLight); // Normalize for accurate calculation of correlation
    
    Vector3f resultingLightIntensity = lightIntenstiy * std::abs(Dot(pointToLight, lightDirection)) * size * size / (distanceToLight * distanceToLight);

    pointToLight = -pointToLight;

    float dp = Dot(pointToLight, target.n); // Correlation between the light direction and the normal

    if (dp <= 0) // The are at right angles or light intersects with the back face of the object
        return outColor;


    Vector3f regulatedLightIntensity = resultingLightIntensity; // Resulting light intensity at the intersection point

    // Diffuse Reflection

    Vector3f resultingDp = (target.mat->DRC + texColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_KD)
        outColor = outColor + (texColor)*regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else if (tex != nullptr && tex->decalMode == Texture::DecalMode::BLEND_KD)
        outColor = outColor + resultingDp * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else
        outColor = outColor + target.mat->DRC * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));



    // Specular Reflection
    Vector3f add = pointToLight + viewerDirection; // Halfway vector
    Vector3f halfwayVector = Normalize(add);

    dp = Dot(halfwayVector, target.n); // Correlation between halfway vector and the normal
    if (dp > 0)
        outColor = outColor + regulatedLightIntensity * static_cast<float>(std::pow(dp, target.mat->phongExponent)) * target.mat->SRC; // Add specular reflection

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_ALL)
        outColor = texColor;

    return outColor;
}
// --

// Environment Light
EnvironmentLight::EnvironmentLight(Texture* _tex)
    : tex(_tex) { lType = LightType::ENVIRONMENT; }

Vector3f EnvironmentLight::GetLightDirection(Vector3f normal)
{
    resultingParams.x = (float)randPoint(0.0f, 1.0f);
    resultingParams.x = resultingParams.x * 2 - 1;
    resultingParams.y = (float)randPoint(0.0f, 1.0f);
    resultingParams.y = resultingParams.y * 2 - 1;
    resultingParams.z = (float)randPoint(0.0f, 1.0f);
    resultingParams.z = resultingParams.z * 2 - 1;

    while (SqLength(resultingParams) > 1 && Dot(resultingParams, normal) <= 0)
    {
        resultingParams.x = (float)randPoint(0.0f, 1.0f);
        resultingParams.x = resultingParams.x * 2 - 1;
        resultingParams.y = (float)randPoint(0.0f, 1.0f);
        resultingParams.y = resultingParams.y * 2 - 1;
        resultingParams.z = (float)randPoint(0.0f, 1.0f);
        resultingParams.z = resultingParams.z * 2 - 1;
    }

    Vector3f res = resultingParams;

    resultingParams = Normalize(resultingParams);

    return res;
}

Vector3f EnvironmentLight::ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, Texture *tex, float gamma)
{
    float gam = 1.0f;
    if(target.mat->degamma)
        gam = gamma;

    Vector3f texColor{};
    if (tex != nullptr)
    {
        if (tex->texType == Texture::TextureType::IMAGE)
        {
            texColor = tex->RetrieveRGBFromUV(target.uv.x, target.uv.y);
            texColor /= tex->normalizer;
        }
        else
            texColor = tex->RetrieveRGBFromUV(target.ip.x, target.ip.y, target.ip.z);
    }

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_ALL)
        return texColor;

    Vector3f outColor(0.0f, 0.0f, 0.0f);

    int mIndex = MinAbsElementIndex(target.n); // Get minimum absolute index

    // Create an orthonormal basis using vrd
    Vector3f rprime = target.n;
    rprime[mIndex] = 1.0f;
    rprime = Normalize(rprime);

    Vector3f u = Normalize(Cross(target.n, rprime));
    Vector3f v = Normalize(Cross(target.n, u));
    // --

    float theta = std::acos(Dot(resultingParams, target.n));
    float phi = std::atan2(Dot(resultingParams, v), Dot(resultingParams, u));

    float uu = (-phi + PI) / (2 * PI);
    float vv = theta / PI;

    Vector3f res = tex->RetrieveRGBFromUV(uu, vv, 0.0f);
    Vector3f regulatedLightIntensity = res * 2 * 3.14159265f;

    Vector3f pointToLight = resultingParams;

    float dp = Dot(pointToLight, target.n); // Correlation between the light direction and the normal

    if (dp <= 0) // The are at right angles or light intersects with the back face of the object
        return outColor;

    Vector3f resultingDp = (target.mat->DRC + texColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_KD)
        outColor = outColor + (texColor)*regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else if (tex != nullptr && tex->decalMode == Texture::DecalMode::BLEND_KD)
        outColor = outColor + resultingDp * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else
        outColor = outColor + target.mat->DRC * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));

    // Specular Reflection
    Vector3f add = pointToLight + viewerDirection; // Halfway vector
    Vector3f halfwayVector = Normalize(add);

    dp = Dot(halfwayVector, target.n); // Correlation between halfway vector and the normal
    if (dp > 0)
        outColor = outColor + regulatedLightIntensity * static_cast<float>(std::pow(dp, target.mat->phongExponent)) * target.mat->SRC * static_cast<float>(std::pow(1, gam)); // Add specular reflection

    return outColor;
}
// -- 

// Point Light
PointLight::PointLight(const Vector3f &position, const Vector3f &intensity)
    : Light(position, intensity) { lType = LightType::POINT; }

Vector3f PointLight::ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, Texture *tex, float gamma)
{
    float gam = 1.0f;
    if (target.mat->degamma)
        gam = gamma;
    Vector3f texColor{};
    if(tex != nullptr)
    {
        if(tex->texType == Texture::TextureType::IMAGE)
        {
            texColor = tex->RetrieveRGBFromUV(target.uv.x, target.uv.y);
            texColor /= tex->normalizer;
            // texColor = tex->GetPixelOffset(target.uv.x, target.uv.y, 0, 0) / 255;
        }
        else
            texColor = tex->RetrieveRGBFromUV(target.ip.x, target.ip.y, target.ip.z); 
    }

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_ALL)
        return texColor;

    Vector3f outColor(0.0f, 0.0f, 0.0f);



    Vector3f pointToLight = lightPosition - target.ip; // Light direction

    float distanceToLight = SqLength(pointToLight); // Get the distance

    pointToLight = Normalize(pointToLight); // Normalize for accurate calculation of correlation

    float dp = Dot(pointToLight, target.n); // Correlation between the light direction and the normal
    
    if (dp <= 0) // The are at right angles or light intersects with the back face of the object
        return outColor;

    Vector3f regulatedLightIntensity = lightIntenstiy / (distanceToLight * distanceToLight); // Resulting light intensity at the intersection point

    // Diffuse Reflection

    Vector3f resultingDp = (target.mat->DRC + texColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    if (tex != nullptr && tex->decalMode == Texture::DecalMode::REPLACE_KD)
        outColor = outColor + (texColor)*regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else if (tex != nullptr && tex->decalMode == Texture::DecalMode::BLEND_KD)
        outColor = outColor + resultingDp * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));
    else
        outColor = outColor + target.mat->DRC * regulatedLightIntensity * dp * static_cast<float>(std::pow(1, gam));

    // Specular Reflection
    Vector3f add = pointToLight + viewerDirection; // Halfway vector
    Vector3f halfwayVector = Normalize(add);

    dp = Dot(halfwayVector, target.n); // Correlation between halfway vector and the normal
    if (dp > 0)
        outColor = outColor + regulatedLightIntensity * static_cast<float>(std::pow(dp, target.mat->phongExponent)) * target.mat->SRC * static_cast<float>(std::pow(1, gam)); // Add specular reflection


    return outColor;
}
// --


}
