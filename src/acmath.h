#ifndef _ACMATH_H_
#define _ACMATH_H_

#include <cmath>
#include <iostream>
#include <vector>
#include <cassert>
#include <limits>
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"

#include <unordered_map>

namespace actracer {

class Material;
class Shape;
union Color;

// 2D vector to represent
// uv coordinates and 2D points
template <typename T>
class Vector2
{
public:
    union {
        T x;
        T s;
        T u;
    };
    union {
        T y;
        T t;
        T v;
    };

    #pragma region constructor and assignment operator

    ~Vector2<T>() = default;

    Vector2<T>(T _x = 0, T _y = 0)
        : x(_x), y(_y) {}

    Vector2<T>(const Vector2<T> &rh)
        : x(rh.x), y(rh.y) {}

    Vector2<T>(const glm::vec2 &v)
        : x(v.x), y(v.y) {}

    Vector2<T> &operator=(const Vector2<T> &rh)
    {
        x = rh.x;
        y = rh.y;

        return *this;
    }

    Vector2<T>(Vector2<T> &&rh)
        : x(rh.x), y(rh.y)
    {
        rh.x = 0;
        rh.y = 0;
    }

    Vector2<T> &operator=(Vector2<T> &&rh)
    {
        x = rh.x;
        y = rh.y;

        rh.x = 0;
        rh.y = 0;

        return *this;
    }
    #pragma endregion

    template <typename U>
    Vector2<T> operator/(U scalar) const
    {
        return {x / scalar, y / scalar};
    }
    template <typename U>
    Vector2<T> operator*(U scalar) const { return {x * scalar, y * scalar}; }
    template <typename U>
    Vector2<T> operator+(U scalar) const { return {x + scalar, y + scalar}; }
    template <typename U>
    Vector2<T> operator-(U scalar) const { return {x - scalar, y - scalar}; }

    template <typename U>
    Vector2<T> &operator/=(U scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    template <typename U>
    Vector2<T> &operator*=(U scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    template <typename U>
    Vector2<T> &operator+=(U scalar)
    {
        x += scalar;
        y += scalar;
        return *this;
    }
    template <typename U>
    Vector2<T> &operator-=(U scalar)
    {
        x -= scalar;
        y -= scalar;
        return *this;
    }

    Vector2<T> operator/=(const Vector2<T> &v)
    {
        x /= v.x;
        y /= v.y;
        return *this;
    }
    Vector2<T> operator*=(const Vector2<T> &v)
    {
        x *= v.x;
        y *= v.y;
        return *this;
    }
    Vector2<T> operator+=(const Vector2<T> &v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }
    Vector2<T> operator-=(const Vector2<T> &v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2<T> operator/(const Vector2<T> &v) const { return {x / v.x, y / v.y}; }
    Vector2<T> operator*(const Vector2<T> &v) const { return {x * v.x, y * v.y}; }
    Vector2<T> operator+(const Vector2<T> &v) const { return {x + v.x, y + v.y}; }
    Vector2<T> operator-(const Vector2<T> &v) const { return {x - v.x, y - v.y}; }

    Vector2<T> operator-() const { return {-x, -y}; }
    
    bool operator==(const Vector2<T>& v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vector2<T>& v) const { return !(*this == v); }

    T operator[](int i) const
    {
        assert(i == 0 || i == 1);
        if (!i)
            return x;

        return y;
    }

    T &operator[](int i)
    {
        assert(i == 0 || i == 1);
        if (!i)
            return x;

        return y;
    }

    template<typename U>
    void SetValues(U arr[2]) { x = arr[0]; y = arr[1]; }


    operator glm::vec2() const { return glm::vec2(x, y); }

};

// 3D vector to represent
// normals, vectors, points etc.
template <typename T>
class Vector3
{
public:
    union {
        T x;
        T r;
    };
    union {
        T y;
        T g;
    };
    union {
        T z;
        T b;
    };


    #pragma region constructors and assignment operators

    ~Vector3<T>() = default;

    // Construct using a Vector2 and a value
    Vector3<T>(const Vector2<T> &v, T _z = 0)
        : x(v.x), y(v.y), z(_z) {}

    // Construct with values
    Vector3<T>(T _x = 0, T _y = 0, T _z = 0)
        : x(_x), y(_y), z(_z) {}

    Vector3<T>(const glm::vec3 &v)
        : x(v.x), y(v.y), z(v.z) {}

    // Copy constructor and assignment
    Vector3<T>(const Vector3<T> &rh)
        : x(rh.x), y(rh.y), z(rh.z) {}

    Vector3<T> &operator=(const Vector3<T> &rh)
    {
        x = rh.x;
        y = rh.y;
        z = rh.z;

        return *this;
    }
    // --

    // Move constructor and assignment
    Vector3<T>(Vector3<T> &&rh)
        : x(rh.x), y(rh.y), z(rh.z)
    {
        rh.x = 0;
        rh.y = 0;
        rh.z = 0;
    }

    Vector3<T> &operator=(Vector3<T> &&rh)
    {
        x = rh.x;
        y = rh.y;
        z = rh.z;

        rh.x = 0;
        rh.y = 0;
        rh.z = 0;

        return *this;
    }
    // --
    #pragma endregion

    template <typename U>
    Vector3<T> &operator/=(U scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
    template <typename U>
    Vector3<T> &operator*=(U scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    template <typename U>
    Vector3<T> &operator+=(U scalar)
    {
        x += scalar;
        y += scalar;
        z += scalar;
        return *this;
    }
    template <typename U>
    Vector3<T> &operator-=(U scalar)
    {
        x -= scalar;
        y -= scalar;
        z -= scalar;
        return *this;
    }

    Vector3<T> operator/=(const Vector3<T> &v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }
    Vector3<T> operator*=(const Vector3<T> &v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    Vector3<T> operator+=(const Vector3<T> &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    Vector3<T> operator-=(const Vector3<T> &v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    template <typename U>
    Vector3<T> operator/(U scalar) const { return {x / scalar, y / scalar, z / scalar}; }
    template <typename U>
    Vector3<T> operator*(U scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    template <typename U>
    Vector3<T> operator+(U scalar) const { return {x + scalar, y + scalar, z + scalar}; }
    template <typename U>
    Vector3<T> operator-(U scalar) const { return {x - scalar, y - scalar, z - scalar}; }

    template <typename U>
    Vector3<T> operator/(const Vector3<U> &v) const { return {x / v.x, y / v.y, z / v.z}; }
    template <typename U>
    Vector3<T> operator*(const Vector3<U> &v) const { return {x * v.x, y * v.y, z * v.z}; }
    template <typename U>
    Vector3<T> operator+(const Vector3<U> &v) const { return {x + v.x, y + v.y, z + v.z}; }
    template <typename U>
    Vector3<T> operator-(const Vector3<U> &v) const { return {x - v.x, y - v.y, z - v.z}; }

    Vector3<T> operator/(const Vector3<T> &v) const { return {x / v.x, y / v.y, z / v.z}; }
    Vector3<T> operator*(const Vector3<T> &v) const { return {x * v.x, y * v.y, z * v.z}; }
    Vector3<T> operator+(const Vector3<T> &v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3<T> operator-(const Vector3<T> &v) const { return {x - v.x, y - v.y, z - v.z}; }

    Vector3<T> operator-() const { return {-x, -y, -z}; }

    bool operator==(const Vector3<T> &v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vector3<T>& v) const { return !(*this == v); }


    T operator[](int i) const
    {
        assert(i >= 0 && i <= 2);

        if (i == 0)
            return x;
        if (i == 1)
            return y;
        if (i == 2)
            return z;
    }

    T &operator[](int i)
    {
        assert(i >= 0 && i <= 2);

        if (i == 0)
            return x;
        if (i == 1)
            return y;
        if (i == 2)
            return z;
    }

    template<typename U>
    void SetValues(U arr[3]) { x = arr[0]; y = arr[1]; z = arr[2]; }

    operator glm::vec3() const { return glm::vec3(x, y, z); }
};

// 4D vector to represent 3D vectors
// with homogenous coordinates
template <typename T>
class Vector4
{
public:
    union {
        T x;
        T r;
    };
    union {
        T y;
        T g;
    };
    union {
        T z;
        T b;
    };
    union {
        T w;
        T a;
    };

    #pragma region constructors and assignment operators
    ~Vector4<T>() = default;

    Vector4<T>(T arr[4]) // Construct with array
        : x(arr[0]), y(arr[1]), z(arr[2]), w(arr[3]) { }

    Vector4<T>(T _x = 0, T _y = 0, T _z = 0, T _w = 0) // Construct with values
        : x(_x), y(_y), z(_z), w(_w) { }

    // Construct using Vector3 and a value
    Vector4<T>(const Vector3<T>& v, T _w = 0)
        : x(v.x), y(v.y), z(v.z), w(_w) { }
    
    // Construct using Vector2 and two values
    Vector4<T>(const Vector2<T>& v, T _z = 0, T _w = 0)
        : x(v.x), y(v.y), z(_z), w(_w) { }


    Vector4<T>(const glm::vec4& v) 
        : x(v.x), y(v.y), z(v.z), w(v.w) { }

    // Copy constructor and assignment
    Vector4<T>(const Vector4<T> &rh) 
        : x(rh.x), y(rh.y), z(rh.z), w(rh.w) { }

    Vector4<T> &operator=(const Vector4<T> &rh)
    {
        x = rh.x;
        y = rh.y;
        z = rh.z;
        w = rh.w;

        return *this;
    }
    // --

    // Move constructor and assignment
    Vector4<T>(Vector4<T> &&rh)
        : x(rh.x), y(rh.y), z(rh.z), w(rh.w)
    {
        rh.x = 0;
        rh.y = 0;
        rh.z = 0;
        rh.w = 0;
    }

    Vector4<T> &operator=(Vector4<T> &&rh)
    {
        x = rh.x;
        y = rh.y;
        z = rh.z;
        w = rh.w;

        rh.x = 0;
        rh.y = 0;
        rh.z = 0;
        rh.w = 0;

        return *this;
    }
    // --

    #pragma endregion

    template<typename U>
    Vector4<T>& operator/=(U scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
    template<typename U>
    Vector4<T>& operator*=(U scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    template<typename U>
    Vector4<T>& operator+=(U scalar) { x += scalar; y += scalar; z += scalar; w += scalar; return *this; }
    template<typename U>
    Vector4<T>& operator-=(U scalar) { x -= scalar; y -= scalar; z -= scalar; w -= scalar; return *this; }

    Vector4<T> operator/=(const Vector4<T> &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
    Vector4<T> operator*=(const Vector4<T> &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vector4<T> operator+=(const Vector4<T> &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vector4<T> operator-=(const Vector4<T> &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }

    template <typename U>
    Vector4<T> operator/(U scalar) const { return {x / scalar, y / scalar, z / scalar, w / scalar}; }
    template <typename U>
    Vector4<T> operator*(U scalar) const { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
    template <typename U>
    Vector4<T> operator+(U scalar) const { return {x + scalar, y + scalar, z + scalar, w + scalar}; }
    template <typename U>
    Vector4<T> operator-(U scalar) const { return {x - scalar, y - scalar, z - scalar, w - scalar}; }

    Vector4<T> operator/(const Vector4<T> &v) const { return {x / v.x, y / v.y, z / v.z, w / v.w}; }
    Vector4<T> operator*(const Vector4<T> &v) const { return {x * v.x, y * v.y, z * v.z, w * v.w}; }
    Vector4<T> operator+(const Vector4<T> &v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    Vector4<T> operator-(const Vector4<T> &v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }

    Vector4<T> operator-() const { return {-x, -y, -z, -w}; }

    bool operator==(const Vector4<T> &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const Vector4<T> &v) const { return !(*this == v); }

    T operator[](int i) const 
    { 
        assert(i >= 0 && i <= 3); 

        if(i == 0) return x;
        if(i == 1) return y;
        if(i == 2) return z;
        if(i == 3) return w;
    }

    T& operator[](int i)
    {
        assert(i >= 0 && i <= 3); 

        if(i == 0) return x;
        if(i == 1) return y;
        if(i == 2) return z;
        if(i == 3) return w;
    } 

    template<typename U>
    void SetValues(U arr[4]) { x = arr[0]; y = arr[1]; z = arr[2]; w = arr[3]; }

    inline operator glm::vec4() const { return glm::vec4(x, y, z, w); }
    inline operator Vector3<float>() const { return Vector3<float>(x, y, z); }
};

// Type definitions for the most used ones
typedef Vector4<float> Vector4f;
typedef Vector4<int> Vector4i;

typedef Vector3<float> Vector3f;
typedef Vector3<int> Vector3i;

typedef Vector2<float> Vector2f;
typedef Vector2<int> Vector2i;

// Overloads for debugging
template<typename T>
inline std::ostream& operator<<(std::ostream& out, const Vector4<T>& v) 
{
    out << "[ " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " ]\n";
    return out;
}
template<typename T>
inline std::ostream& operator<<(std::ostream& out, const Vector3<T>& v)
{
    out << "[ " << v.x << ", " << v.y << ", " << v.z << " ]\n";
    return out;
}
template<typename T>
inline std::ostream& operator<<(std::ostream& out, const Vector2<T>& v)
{
    out << "[ " << v.x << ", " << v.y << " ]\n";
    return out;
}

template <typename T>
inline T Dot(const Vector4<T> &v0, const Vector4<T> &v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;  }

template<typename T>
inline T Dot(const Vector3<T>& v0, const Vector3<T>& v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z; }

template <typename T>
inline T Dot(const Vector2<T> &v0, const Vector2<T> &v1) { return v0.x * v1.x + v0.y * v1.y; }

template<typename T>
inline Vector3<T> Cross(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return Vector3<T>{ 
            v0.y * v1.z - v0.z * v1.y,
            v0.z * v1.x - v0.x * v1.z,
            v0.x * v1.y - v0.y * v1.x 
            };
}

template<typename T>
inline T Length(const Vector4<T>& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }

template <typename T>
inline T Length(const Vector3<T> &v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

template <typename T>
inline T Length(const Vector2<T> &v) { return v.x * v.x + v.y * v.y; }

template <typename T>
inline T SqLength(const Vector4<T> &v) { return std::sqrt(Length(v)); }

template <typename T>
inline T SqLength(const Vector3<T> &v) { return std::sqrt(Length(v)); }

template <typename T>
inline T SqLength(const Vector2<T> &v) { return std::sqrt(Length(v)); }

template<typename T>
inline Vector4<T> Normalize(const Vector4<T>& v) { float l = SqLength(v); return v / l; }

template<typename T>
inline Vector3<T> Normalize(const Vector3<T>& v) { float l = SqLength(v); return v / l; }

template<typename T>
inline Vector2<T> Normalize(const Vector2<T>& v) { float l = SqLength(v); return v / l; }

template<typename T>
inline T MaxElement(const Vector3<T>& v) { return v.x > v.y ? (v.x > v.z ? v.x : v.z) : (v.y > v.z ? v.y : v.z); }

template <typename T>
inline T MaxElement(const Vector2<T> &v) { return v.x > v.y ? v.x : v.y; }

template<typename T>
inline T MinElement(const Vector3<T>& v) { return v.x < v.y ? (v.x < v.z ? v.x : v.z) : (v.y < v.z ? v.y : v.z ); }

template <typename T>
inline T MinElement(const Vector2<T> &v) { return v.x < v.y ? v.x : v.y; }

template<typename T>
inline T MaxElementIndex(const Vector3<T>& v) { return v.x > v.y ? (v.x > v.z ? 0 : 2) : (v.y > v.z ? 1 : 2); }

template<typename T>
inline T MaxElementIndex(const Vector2<T>& v) { return v.x > v.y ? 0 : 1; }

template<typename T>
inline int MinElementIndex(const Vector3<T>& v) { return v.x < v.y ? (v.x < v.z ? 0 : 2) : (v.y < v.z ? 1 : 2); }

template<typename T>
inline int MinElementIndex(const Vector2<T>& v) { return v.x < v.y ? 0 : 1; }

template <typename T>
inline int MinAbsElementIndex(const Vector3<T> &v) { return std::abs(v.x) < std::abs(v.y) ? (std::abs(v.x) < std::abs(v.z) ? 0 : 2) : (std::abs(v.y) < std::abs(v.z) ? 1 : 2); }

template <typename T>
inline int MaxAbsElementIndex(const Vector3<T> &v) { return std::abs(v.x) > std::abs(v.y) ? (std::abs(v.x) > std::abs(v.z) ? 0 : 2) : (std::abs(v.y) > std::abs(v.z) ? 1 : 2); }

template<typename T>
inline Vector3<T> MaxElements(const Vector3<T>& v0, const Vector3<T>& v1) 
{ 
    return { v0.x > v1.x ? v0.x : v1.x,
             v0.y > v1.y ? v0.y : v1.y,
             v0.z > v1.z ? v0.z : v1.z }; 
}

template<typename T>
inline Vector2<T> MaxElements(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return {v0.x > v1.x ? v0.x : v1.x,
            v0.y > v1.y ? v0.y : v1.y};
}

template<typename T>
inline Vector3<T> MinElements(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return {
        v0.x < v1.x ? v0.x : v1.x,
        v0.y < v1.y ? v0.y : v1.y,
        v0.z < v1.z ? v0.z : v1.z
    };
}

template<typename T>
inline Vector2<T> MinElements(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return {
        v0.x < v1.x ? v0.x : v1.x,
        v0.y < v1.y ? v0.y : v1.y};
}

template<typename T>
inline Vector4<T> operator/(T scalar, const Vector4<T>& v) { return v / scalar; }
template <typename T>
inline Vector4<T> operator*(T scalar, const Vector4<T> &v) { return v * scalar; }
template <typename T>
inline Vector4<T> operator+(T scalar, const Vector4<T> &v) { return v + scalar; }
template <typename T>
inline Vector4<T> operator-(T scalar, const Vector4<T> &v) { return v - scalar; }

template <typename T>
inline Vector3<T> operator/(T scalar, const Vector3<T> &v) { return v / scalar; }
template <typename T>
inline Vector3<T> operator*(T scalar, const Vector3<T> &v) { return v * scalar; }
template <typename T>
inline Vector3<T> operator+(T scalar, const Vector3<T> &v) { return v + scalar; }
template <typename T>
inline Vector3<T> operator-(T scalar, const Vector3<T> &v) { return v - scalar; }

template <typename T>
inline Vector2<T> operator/(T scalar, const Vector2<T> &v) { return v / scalar; }
template <typename T>
inline Vector2<T> operator*(T scalar, const Vector2<T> &v) { return v * scalar; }
template <typename T>
inline Vector2<T> operator+(T scalar, const Vector2<T> &v) { return v + scalar; }
template <typename T>
inline Vector2<T> operator-(T scalar, const Vector2<T> &v) { return v - scalar; }

template <typename T>
void SetMax(T& val0, const T& val1) { val0 = val0 < val1 ? val1 : val0; }

template<typename T>
void SetMin(T& val0, const T& val1) { val0 = val0 > val1 ? val1 : val0; }

template<typename T>
T Clamp(T& val, T lowerBound, T upperBound) { return val < lowerBound ? lowerBound : (val > upperBound ? upperBound : val); }

template<typename T>
T Lerp(T minBound, T maxBound, float t) { return (1 - t) * minBound + t * maxBound; }

template<typename T>
float Remap01(T val, T mn, T mx) { return Clamp((val - mn) / (mx - mn), 0.0f, 1.0f); }

inline bool AroundZero(float value, float sensitivity = 0.001f) { return value > -sensitivity && value < sensitivity; }

inline float Smoothstep(const float &t)
{
    return 6 * t * t * t * t * t - 15 * t * t * t * t + 10 * t * t * t;
}

class Transform;

class Ray
{
public:
    Vector3f o; // Origin of the ray
    Vector3f d; // Direction of the ray

    Vector3f ryo;
    Vector3f ryd;

    Vector3f rxo;
    Vector3f rxd;

    Material* currMat;
    Shape*    currShape;

    float time; // Time variable that varies between [0 - 1] range to simulate motion blur effect

    std::unordered_map<Shape*, Ray*> transformedModes;
    std::unordered_map<Shape*, Transform*> transformMatrices;

    void InsertTransform(Shape*, Transform* newTransform);
    void InsertRay(Shape*, Ray* newRay);

    Ray() : o(Vector3f{}), d(Vector3f{}) {}                                             // Default constructor
    Ray(const Vector3f &origin, const Vector3f &direction, Material* m = nullptr, Shape* s = nullptr, float t = 0.0f) : o(origin), d(direction), currMat(m), currShape(s), time(t) {} // Construct with origin, direction
    
    // Takes parameter t and returns a point
    // => o + t * d
    Vector3f operator()(float t) const { return o + d * t; }
    // Takes a point and returns what should be
    // the parameter t of o + t * d to be equal to this point
    // => o + t * d = target -> t = (target - o) / d
    float operator()(const Vector3f &target) const { return (target.x - o.x) / d.x; }
};

// std::ostream& operator<<(std::ostream& out, const Ray& r) { out << "Origin: " << r.o << "Direction: " << r.d; return out; }

// 3D bounding volume for shapes
template<typename T>
class BoundingVolume3 {
public:
    Vector3<T> min; // The point with all minimum values in the x, y, z and axes
    Vector3<T> max; // The point with all maximum values in the x, y, z and axes

    BoundingVolume3() 
    {
        min = Vector3<T>{1e9, 1e9, 1e9};
        max = Vector3<T>{-1e9, -1e9, -1e9};
    }

    BoundingVolume3(const Vector3<T>& v0, const Vector3<T>& v1)
    {
        max = MaxElements(v0, v1);
        min = MinElements(v0, v1);
    }

    // Checks if intersections occurs between a ray and
    // the bounding volume takes Ray and two parameters
    // to define the parameter t of the nearest and farthest intersections
    // with the volume box
    bool Intersect(const Ray& r, float& tn, float& tf)
    {
        // Initialize with min and max values
        tn = 0; // Initialize with 0 since rays go only forward
        tf = std::numeric_limits<float>::max(); // Initialize with inf 
        // --

        // Check for x 
        float tnn = (min.x - r.o.x) / r.d.x;
        float tff = (max.x - r.o.x) / r.d.x;
        
        if(tnn > tff)
            std::swap(tnn, tff);

        SetMax(tn, tnn);
        SetMin(tf, tff);

        if(tf < tn)
            return false;
        // --

        // Check for y
        tnn = (min.y - r.o.y) / r.d.y;
        tff = (max.y - r.o.y) / r.d.y;

        if(tnn > tff)
            std::swap(tnn, tff);

        SetMax(tn, tnn);
        SetMin(tf, tff);

        if(tf < tn)
            return false;
        // --
        
        // Check for z
        tnn = (min.z - r.o.z) / r.d.z;
        tff = (max.z - r.o.z) / r.d.z;

        if(tnn > tff)
            std::swap(tnn, tff);
        
        SetMax(tn, tnn);
        SetMin(tf, tff);

        if(tf < tn)
            return false;
        // -- 

        return true;
    }

    // Returns the volume of the bounding box
    float Volume()
    {
        Vector3f extents = max - min;
        return max.x * max.y * max.z;
    }

    // Returns the surface area of the bounding box
    float SA()
    {
        Vector3f extents = max - min;
        return 2 * ( extents.x * extents.y + extents.y * extents.z + extents.x * extents.z );
    }
};

// 2D bounding volume for planes
template <typename T>
class BoundingVolume2
{
public:
    Vector2<T> min; // The point with all minimum values in the x and y axes
    Vector2<T> max; // The point with all maximum values in the x and y axes

    BoundingVolume2()
    {
        min = Vector2<T>{1e9, 1e9};
        max = Vector2<T>{-1e9, -1e9};
    }

    BoundingVolume2(const Vector2<T> &v0, const Vector2<T> &v1)
    {
        max = MaxElements(v0, v1);
        min = MinElements(v0, v1);
    }

    // Checks if intersections occurs between a ray and
    // the bounding volume takes Ray and two parameters
    // to define the parameter t of the nearest and farthest intersections
    // with the volume box
    bool Intersect(const Ray &r, float &tn, float &tf)
    {
        // Initialize with min and max values
        tn = 0;                                 // Initialize with 0 since rays go only forward
        tf = std::numeric_limits<float>::max(); // Initialize with inf
        // --

        // Check for x
        float tnn = (min.x - r.o.x) / r.d.x;
        float tff = (max.x - r.o.x) / r.d.x;

        if (tnn > tff)
            std::swap(tnn, tff);

        SetMax(tn, tnn);
        SetMin(tf, tff);

        if (tf < tn)
            return false;
        // --

        // Check for y
        tnn = (min.y - r.o.y) / r.d.y;
        tff = (max.y - r.o.y) / r.d.y;

        if (tnn > tff)
            std::swap(tnn, tff);

        SetMax(tn, tnn);
        SetMin(tf, tff);

        if (tf < tn)
            return false;
        // --

        return true;
    }
    
    // Returns relative position of the ith square if we were to cut
    // it into partitionCount X partitionCount matrix
    std::pair<float, float> FindIthPartition(int id, int partitionCount, float partitionOffset) const
    {
        int horzPartition = id % partitionCount;
        int vertPartition = id / partitionCount;
        
        return std::make_pair(horzPartition * partitionOffset, vertPartition * partitionOffset);
    }

    Vector2f RelativenessToCenter(const Vector2f& v) const { return v - 0.5f; }
};

template<typename T>
std::ostream& operator<<(std::ostream& out, BoundingVolume2<T>& b) { out << b.min << b.max; return out; }

template<typename T>
std::ostream& operator<<(std::ostream& out, BoundingVolume3<T>& b) { out << b.min << b.max; return out; }

// Merge two bounds and return
// Returns a new bound with maximum extents
// of the given two combined
template <typename T>
BoundingVolume3<T> Merge(const BoundingVolume3<T> &b0, const BoundingVolume3<T> &b1)
{ return BoundingVolume3<T>(MaxElements(b0.max, b1.max), MinElements(b0.min, b1.min)); }

template <typename T>
BoundingVolume3<T> Merge(const BoundingVolume2<T> &b0, const BoundingVolume2<T> &b1)
{ return BoundingVolume2<T>(MaxElements(b0.max, b1.max), MinElements(b0.min, b1.min)); }

// Merge bound a vector max of two will be the max point
// min of two will be the min point
template<typename T>
BoundingVolume3<T> Merge(const BoundingVolume3<T>& b, const Vector3<T>& v)
{ return BoundingVolume3<T>(MaxElements(b.max, v), MinElements(b.min, v)); }

template<typename T>
BoundingVolume2<T> Merge(const BoundingVolume2<T>& b, const Vector2<T>& v)
{ return BoundingVolume2<T>(MaxElements(b.max, v), MinElements(b.min, v)); }

// Find the value of the v's axis in the bouding volume
// it will be in terms of => [b.min, b.max] -> [0, 1]
template <typename T>
float ClippedPosition(const BoundingVolume3<T>& b, const Vector3<T>& v, int ax)
{ return (v[ax] - b.min[ax]) / (b.max[ax] - b.min[ax]); }

template <typename T>
float ClippedPosition(const BoundingVolume2<T>& b, const Vector2<T>& v, int ax)
{ return (v[ax] - b.min[ax]) / (b.max[ax] - b.min[ax]); }

typedef BoundingVolume3<float> BoundingVolume3f;
typedef BoundingVolume3<int>   BoundingVolume3i;

typedef BoundingVolume2<float> BoundingVolume2f;
typedef BoundingVolume2<int>   BountdingVolume2i;

static std::ostream& operator<<(std::ostream& out, const glm::vec4& v) { out << "[ " << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << "]\n"; return out; }
static std::ostream& operator<<(std::ostream& out, const glm::mat4& m) { out << "[\n  " << m[0] << "  " << m[1] << "  " << m[2] << "  " << m[3] << "]\n"; return out; }

struct Transformation
{
    int id;
    glm::mat4 transformation;

    Transformation(int _id) : id(_id) { }
};

struct Scaling : Transformation
{
    Scaling(int _id, float x, float y, float z) : Transformation(_id) { transformation = glm::scale(glm::mat4(1), glm::vec3(x, y, z)); }
    Scaling(int _id, const glm::vec3 &v) : Transformation(_id) { transformation = glm::scale(glm::mat4(1), v); }
};

struct Rotation : Transformation
{
    Rotation(int _id, glm::vec3 axis, float radAngle) : Transformation(_id) { transformation = glm::rotate(glm::mat4(1), radAngle * (3.14f / 180.0f), axis); }
};

struct Translation : Transformation
{
    Translation(int _id, float x, float y, float z) : Transformation(_id) { transformation = glm::translate(glm::mat4(1), glm::vec3(x, y, z)); }
    Translation(int _id, const glm::vec3 &v) : Transformation(_id) { transformation = glm::translate(glm::mat4(1), v); }
};

class Transform {
public:
    glm::mat4 transformationMatrix;
    glm::mat4 invTransformationMatrix;
    glm::mat4 transposeMatrix;

    Transform() {}

    Transform(glm::mat4& pMatrix)
        : transposeMatrix(pMatrix)
    {
        transformationMatrix = glm::transpose(transposeMatrix);
        invTransformationMatrix = glm::inverse(transformationMatrix);
    }

    void UpdateTransform()
    {
        transformationMatrix = glm::transpose(transposeMatrix);
        invTransformationMatrix = glm::inverse(transformationMatrix);
    }

    Transform operator()(const Transformation& other)
    {
        glm::mat4 newTransformationMatrix = other.transformation * this->transposeMatrix;
        return std::move(Transform(newTransformationMatrix));
    }

    Transform operator()(const Transform &other)
    {
        glm::mat4 newTransformationMatrix = other.transposeMatrix * this->transposeMatrix;
        return Transform(newTransformationMatrix);
    }

    Vector4f operator()(Vector4f vec, bool obj2world = true, bool isDirection = false) const 
    {
        glm::vec4 glvec = glm::vec4(vec.x, vec.y, vec.z, vec.w);

        if(isDirection)
        {
            if (obj2world)
                return this->invTransformationMatrix * static_cast<glm::vec4>(vec);
            else
                return glm::transpose(this->invTransformationMatrix) * static_cast<glm::vec4>(vec);
        }

        if(obj2world) 
            return this->transposeMatrix * static_cast<glm::vec4>(vec);
        else
            return glm::transpose(this->invTransformationMatrix) * static_cast<glm::vec4>(vec);
    }

    BoundingVolume3f operator()(const BoundingVolume3f &bbox, bool obj2world = true)
    {
        const Transform& temp = *this;

        Vector3f v0 = static_cast<Vector3f>(temp(Vector4f(bbox.min, 1.0f)));
        
        BoundingVolume3f res(v0, v0);
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.max.x, bbox.min.y, bbox.min.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.min.x, bbox.max.y, bbox.min.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.min.x, bbox.min.y, bbox.max.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.min.x, bbox.max.y, bbox.max.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.max.x, bbox.max.y, bbox.min.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.max.x, bbox.min.y, bbox.max.z, 1.0f))));
        res = Merge(res, static_cast<Vector3f>(temp(Vector4f(bbox.max.x, bbox.max.y, bbox.max.z, 1.0f))));

        return res;
    }
    // Transpose transformation
    // Ray transformation
    Ray operator()(const Ray &r, bool obj2world = true)
    {
        Ray resRay{};
        resRay.currMat = r.currMat;
        resRay.currShape = r.currShape;
        resRay.time = r.time;

        Vector4f resultRayOrigin = (*this)(Vector4f(r.o, 1.0f), obj2world, false);
        Vector4f resultRayDirection = (*this)(Vector4f(r.d, 0.0f), obj2world, false);

        resRay.o = Vector3f(resultRayOrigin.x, resultRayOrigin.y, resultRayOrigin.z);
        resRay.d = Vector3f(resultRayDirection.x, resultRayDirection.y, resultRayDirection.z);

        resRay.d = Normalize(resRay.d);

        return resRay;
    }
};

static float CalculateGaussian(float x, float y, float spreadness)
{ return (1.0f / (2 * 3.14f * spreadness)) * std::exp(-(x * x, y * y) / (2 * spreadness * spreadness)); }


// Vertex structure to use in the triangles
struct Vertex
{
    Vector3f p{};
    Vector3f n{};
    Vector2f uv{};

    int refCount = 0;
};

}



#endif
