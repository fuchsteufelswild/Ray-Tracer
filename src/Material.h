#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "acmath.h"


namespace actracer {

class BRDFBase;

class Material {
public:
    enum MatType { DEFAULT, MIRROR, DIELECTRIC, CONDUCTOR };
public:
    static Material* DefaultMaterial;

public:
    Material() : rIndex(1), att{}, phongExponent(0), mType(MatType::DEFAULT) { };

    Material(int _id, MatType _mType, const Vector3f& _DRC, const Vector3f& _SRC, const Vector3f& _ARC, const Vector3f& _MRC, float _rIndex, float _att, int _phong, float _roughness)
        : id(_id), mType(_mType), DRC(_DRC), SRC(_SRC), ARC(_ARC), MRC(_MRC), rIndex(_rIndex), att(_att), phongExponent(_phong), roughness(_roughness) { }
public:
    /*
     * Using fresnel formulas computes the fresnel value
     * Dielectric and Conductor materials computes it differently
     * Refer to internet for formulas
     */ 
    virtual float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2);

    virtual Vector3f ACR() {return Vector3f{}; }

public:
    Vector3f GetDiffuseReflectionCoefficient() const;
    Vector3f GetSpecularReflectionCoefficient() const;
    Vector3f GetAmbientReflectionCoefficient() const;
    Vector3f GetMirrorReflectionCoefficient() const;
    BRDFBase *GetBRDF() const;

    float GetRoughness() const;
    float GetRefractionIndex() const;

    MatType GetMaterialType() const;
    bool GetDegamma() const;
    int GetID() const;
public:
    void SetBRDF(BRDFBase* brdf);
    void SetDegamma(bool degamma);

private:
    bool degamma = false;
    int id;
    MatType mType;

    Vector3f DRC; // Diffuse reflection coefficient
    Vector3f SRC; // Specular reflection coefficient
    Vector3f ARC; // Ambient reflection coefficient
    Vector3f MRC; // Mirror reflection coefficient

    BRDFBase *brdf;

    float rIndex;      // Refraction index
    float att;         // Attenuation constant
    int phongExponent; // Phong exponent for the specular reflection
    float roughness;
}; 

class Dielectric : public Material {

public:
    Dielectric(int _id, MatType _mType, const Vector3f &_DRC, const Vector3f &_SRC, const Vector3f &_ARC, const Vector3f &_MRC, float _rIndex, float _att, int _phong, float _roughness, const Vector3f &_AC)
        : Material(_id, _mType, _DRC, _SRC, _ARC, _MRC, _rIndex, _att, _phong, _roughness), AC(_AC) {}

    float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2) override;

    Vector3f ACR() { return AC; }

private:
    Vector3f AC; // Absorption coefficient
};

class Conductor : public Material {

public:
    Conductor(int _id, MatType _mType, const Vector3f &_DRC, const Vector3f &_SRC, const Vector3f &_ARC, const Vector3f &_MRC, float _rIndex, float _att, int _phong, float _roughness, float _aIndex)
        : Material(_id, _mType, _DRC, _SRC, _ARC, _MRC, _rIndex, _att, _phong, _roughness), aIndex(_aIndex) {}

    float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2) override;

private:
    float aIndex; // Absorption index
};

inline Vector3f Material::GetDiffuseReflectionCoefficient() const
{
    return DRC;
}

inline Vector3f Material::GetSpecularReflectionCoefficient() const
{
    return SRC;
}

inline Vector3f Material::GetAmbientReflectionCoefficient() const
{
    return ARC;
}

inline Vector3f Material::GetMirrorReflectionCoefficient() const
{
    return MRC;
}

inline BRDFBase *Material::GetBRDF() const
{
    return brdf;
}

inline float Material::GetRoughness() const
{
    return roughness;
}

inline float Material::GetRefractionIndex() const
{
    return rIndex;
}

inline void Material::SetBRDF(BRDFBase *brdf)
{
    this->brdf = brdf;
}

inline void Material::SetDegamma(bool degamma)
{
    this->degamma = degamma;
}

inline Material::MatType Material::GetMaterialType() const
{
    return mType;
}

inline bool Material::GetDegamma() const
{
    return degamma;
}

inline int Material::GetID() const
{
    return id;
}

}

#endif