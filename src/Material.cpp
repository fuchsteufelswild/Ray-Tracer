#include "Material.h"


namespace actracer {

Material* Material::DefaultMaterial = new Material{};

float Material::ComputeFresnelEffect(float n1, float n2, float cos1, float cos2)
{
    return 1.0f;
}

float Dielectric::ComputeFresnelEffect(float n1, float n2, float cos1, float cos2)
{
    float n2c1 = n2 * cos1;
    float n1c2 = n1 * cos2;
    float rpll = (n2c1 - n1c2) / (n2c1 + n1c2); // For parallel component

    float n1c1 = n1 * cos1;
    float n2c2 = n2 * cos2;

    float rperp = (n1c1 - n2c2) / (n1c1 + n2c2);
    return 0.5f * (rpll * rpll + rperp * rperp);
}

float Conductor::ComputeFresnelEffect(float n1, float n2, float cos1, float cos2)
{
    n1 = this->GetRefractionIndex();

    float c2 = cos1 * cos1;
    float nk = n1 * n1 + aIndex * aIndex;
    float n2c = 2 * n1 * cos1;

    float Rs = (nk - n2c + c2) / (nk + n2c + c2);
    float Rp = (nk * c2 - n2c + 1) / (nk * c2 + n2c + 1);

    return 0.5f * (Rs + Rp);
}


}