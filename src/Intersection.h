#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

#include "acmath.h"
#include <vector>



namespace actracer {

class Material;
class Shape;
class Texture;

// Base intersection class for storing the information
// about the intersection between rays and entities
class Intersection {
public:
    Vector3f  lip;  // Local Intersection point
    Vector3f  ip;   // Intersection point
    Vector3f  n;    // Normal of the surface point
    Vector2f  uv;   // UV of the surface point
    Vector3f  rd;   // Direction of the ray intersected the surface
    float     t;    // Parameter t of the intersected ray's equation o + t * d = ip 
    Material* mat;  // Material of the surface
    

    Intersection() : mat(nullptr), t(std::numeric_limits<float>::max()) {}

    Intersection(const Vector3f& _lip, const Vector3f& _ip, const Vector3f& _n, const Vector2f _uv, Vector3f _rd, float _t = std::numeric_limits<float>::max(), Material* _mat = nullptr)
        : lip(_lip), ip(_ip), n(_n), uv(_uv), rd(_rd), t(_t), mat(_mat) { }
};

// Intersection type used for ray-shape intersections
// Additionaly stores the information about the shape
class SurfaceIntersection : public Intersection {
public:
    Shape* shape; // The object which the ray intersected with

    Texture *tex1;
    Texture *tex2;

    SurfaceIntersection() : Intersection() { }

    SurfaceIntersection(const Vector3f& _lip, const Vector3f &_ip, const Vector3f &_n, const Vector2f _uv, Vector3f _rd, float _t = std::numeric_limits<float>::max(), Material *_mat = nullptr, Shape *_s = nullptr, Texture* text1 = nullptr, Texture* tex2 = nullptr)
        : Intersection(_lip, _ip, _n, _uv, _rd, _t, _mat), shape(_s), tex1(text1), tex2(tex2) 
    { }
};

}

#endif