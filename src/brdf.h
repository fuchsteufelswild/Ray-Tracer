#ifndef _BRDF_H_
#define _BRDF_H_

#include "acmath.h"


namespace actracer {

class BRDFBase {
protected:
    float specularPhong;
public:
    float ai;
    virtual Vector3f f(const Vector3f& wi, const Vector3f& wo, const Vector3f& n, const Vector3f& kd, const Vector3f& ks, float ri = 0.0f) = 0;

    BRDFBase(float phong) : specularPhong(phong) { }
};

class BRDFBlinnPhongModified : public BRDFBase{
private:
    bool normalized;

    void NormalizedCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f& result);
    void ClassicCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f& result);
public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFBlinnPhongModified(float phong, bool _normalized = false) : BRDFBase(phong), normalized(_normalized) { }
};

class BRDFBlinnPhongModifiedNormalized : public BRDFBase{

public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFBlinnPhongModifiedNormalized(float phong) : BRDFBase(phong) {}
};

class BRDFBlinnPhongOriginal : public BRDFBase {

public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFBlinnPhongOriginal(float phong) : BRDFBase(phong) {}
};

class BRDFPhongModified : public BRDFBase {
private:
    bool normalized;

    void NormalizedCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f &result);
    void ClassicCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f &result);

public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFPhongModified(float phong, bool _normalized = false) : BRDFBase(phong), normalized(_normalized) {}
};

class BRDFPhongModifiedNormalized : public BRDFBase {

public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFPhongModifiedNormalized(float phong) : BRDFBase(phong) {}
};

class BRDFPhongOriginal : public BRDFBase {
    
public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFPhongOriginal(float phong) : BRDFBase(phong) {}
};

class BRDFTorranceSparrow : public BRDFBase {
private:
    float ri; // index of refraction
    bool kdfresnel;

    
private:
    float d(float cosa);
    float fr(float cosb, float ri);
    float g(const Vector3f& wi, const Vector3f& wo, const Vector3f& n);
public:
    virtual Vector3f f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri = 0.0f) override;

    BRDFTorranceSparrow(float phong, bool _kdfresnel = false) : BRDFBase(phong), kdfresnel(_kdfresnel) {}
};

}




#endif