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
    bool degamma = false;
    int id;
    MatType mType;

    Vector3f DRC; // Diffuse reflection coefficient
    Vector3f SRC; // Specular reflection coefficient
    Vector3f ARC; // Ambient reflection coefficient
    Vector3f MRC; // Mirror reflection coefficient
    BRDFBase* brdf;

    float rIndex; // Refraction index
    float att;    // Attenuation constant
    float roughness;
    
    int phongExponent; // Phong exponent for the specular reflection

    Material() : rIndex(1), att{}, phongExponent(0), mType(MatType::DEFAULT) { };

    Material(int _id, MatType _mType, const Vector3f& _DRC, const Vector3f& _SRC, const Vector3f& _ARC, const Vector3f& _MRC, float _rIndex, float _att, int _phong, float _roughness)
        : id(_id), mType(_mType), DRC(_DRC), SRC(_SRC), ARC(_ARC), MRC(_MRC), rIndex(_rIndex), att(_att), phongExponent(_phong), roughness(_roughness) { }

    /*
     * Using fresnel formulas computes the fresnel value
     * Dielectric and Conductor materials computes it differently
     * Refer to internet for formulas
     */ 
    virtual float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2);

    virtual Vector3f ACR() {return Vector3f{}; }

}; 

class Dielectric : public Material {
public:
    Vector3f AC; // Absorption coefficient

    Dielectric(int _id, MatType _mType, const Vector3f &_DRC, const Vector3f &_SRC, const Vector3f &_ARC, const Vector3f &_MRC, float _rIndex, float _att, int _phong, float _roughness, const Vector3f &_AC)
        : Material(_id, _mType, _DRC, _SRC, _ARC, _MRC, _rIndex, _att, _phong, _roughness), AC(_AC) {}

    float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2) override;

    Vector3f ACR() { return AC; }
};

class Conductor : public Material {
public:
    float aIndex; // Absorption index

    Conductor(int _id, MatType _mType, const Vector3f &_DRC, const Vector3f &_SRC, const Vector3f &_ARC, const Vector3f &_MRC, float _rIndex, float _att, int _phong, float _roughness, float _aIndex)
        : Material(_id, _mType, _DRC, _SRC, _ARC, _MRC, _rIndex, _att, _phong, _roughness), aIndex(_aIndex) {}

    float ComputeFresnelEffect(float n1, float n2, float cos1, float cos2) override;
};

}

#endif