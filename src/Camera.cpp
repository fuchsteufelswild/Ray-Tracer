#include "Camera.h"

#include <string.h>
#include <iostream>

#include <cmath>
#include <chrono>
#include "Random.h"
#include "Scene.h"

namespace actracer {

// Construct camera object with given parameters
Camera::Camera(int id, const char *imageName, const Vector3f &pos, const Vector3f &gaze, const Vector3f &up, const ImagePlane &imgPlane, int numberOfSamples, float focDis, float apertSize)
    : imgPlane(imgPlane), m_Id(id), m_Pos(pos), m_Gaze(gaze), m_Up(up), focusDistance(focDis), apertureSize(apertSize)

{
    // Set Camera name
    const char* c = imageName;

    char* f = this->imageName;

    while(*c != '\0')
    {
        if(*c != ' ')
            *(f++) = *(c++); 
        else
            c++;
    }

    *f = '\0';
    // --
    
    // Set camera coordinate axes
    m_Gaze = Normalize(m_Gaze);
    m_Up   = Normalize(m_Up);

    float correlation = Dot(m_Gaze, m_Up);
    if(correlation > 0.01f || correlation < -0.01f)
    {
        m_Right = Normalize(Cross(m_Gaze, m_Up));
        m_Up    = Normalize(Cross(m_Right, m_Gaze));
    }
    else
    {
        m_Up = Normalize(m_Up);
        m_Right = Normalize(Cross(m_Gaze, m_Up));
    }
    // --

    nSamples = numberOfSamples;
    nSamplesSqrt = std::sqrt(nSamples);
    sampleHorzDiff = 1.0f / nSamplesSqrt;
    sampleVertDiff = 1.0f / nSamplesSqrt;

    camRandom = Random<double>{};
    rayRandomX = Random<double>{};
    rayRandomY = Random<double>{};

    camSeed = std::chrono::system_clock::now().time_since_epoch().count();
    camGenerator = std::default_random_engine(camSeed);
    camDistribution = std::uniform_real_distribution<double>(-apertureSize / 2, apertSize / 2);

    std::cout << "cam info\n";
    std::cout << m_Pos << m_Gaze << m_Up << m_Right << imgPlane.left << " " << imgPlane.right << " " << imgPlane.bottom << " " << imgPlane.top << " " << imgPlane.distance << "\n";
}

// Takes coordinate of an image pixel as row and col, and
// returns the ray going through that pixel.
Ray Camera::GenerateRay(int row, int col) const
{
    float horizontalOffset = imgPlane.left + (col + 0.5f) * (imgPlane.right - imgPlane.left) / imgPlane.nx; // Calculate horizontal offset from the leftmost point of the image plane
    float verticalOffset = imgPlane.top - (row + 0.5f) * (imgPlane.top - imgPlane.bottom) / imgPlane.ny;    // Calculate vertical offset from the top of the image plane

    Vector3f rayDirection = Normalize(m_Gaze * (imgPlane.distance) + m_Right * horizontalOffset + m_Up * verticalOffset); // Calculate ray direction from camera position to pixel position

    Ray rr(m_Pos, rayDirection); // No need for time no sampling motion blur would be sluggy

    return rr; // Create and return the ray object
}

// Generate ray for a pixel sample
Ray Camera::GenerateRayFromPixelSample(const Pixel& px, int id, PixelSample& pxs)
{
    std::pair<float, float> samplePos = px.pixelBox.FindIthPartition(id, nSamplesSqrt, sampleHorzDiff); // Find the position of the ith partition sample (e.g 6x6 pixel's 20th)

    float randX = rayRandomX(0.0f, sampleHorzDiff);
    float randY = rayRandomY(0.0f, sampleVertDiff);

    if (Scene::debugCurrent >= Scene::debugBegin && Scene::debugCurrent <= Scene::debugEnd)
    {
        std::cout << "Sampled pixel sample pos Before: " << id << " " << samplePos.first << " " << samplePos.second << "\n";
    }

    samplePos.first += randX;
    samplePos.second += randY;
    
    Vector2f relativeToCenter = px.pixelBox.RelativenessToCenter({samplePos.first, samplePos.second});

    pxs.positionRelativeToCenter.x = relativeToCenter.x;
    pxs.positionRelativeToCenter.y = relativeToCenter.y;

    float horizontalOffset = imgPlane.left + (px.col + samplePos.first) * px.calcHorzOffset; // Calculate horizontal offset from the leftmost point of the image plane
    float verticalOffset = imgPlane.top - (px.row + samplePos.second) * px.calcVertOffset;    // Calculate vertical offset from the top of the image plane

    Vector3f rayDirection = Normalize(m_Gaze * (imgPlane.distance) + m_Right * horizontalOffset + m_Up * verticalOffset); // Calculate ray direction from camera position to pixel position

    Ray rr(m_Pos, rayDirection); // Standard ray ( without any tilt by lens )

    if(apertureSize > 0) // Depth of field
    {
        float arandX = camRandom(-apertureSize / 2.0, apertureSize / 2.0);
        float arandY = camRandom(-apertureSize / 2.0, apertureSize / 2.0);

        Vector3f s = this->m_Pos + this->m_Right * arandX + this->m_Up * arandY; // Moved point in the aperture
        float td = this->focusDistance / (Dot(rayDirection, this->m_Gaze)); // t value to reach focal distance with primary ray

        Vector3f p = rr(td); // Hit point on the plane at focal distance from camera
        Vector3f d = Normalize(p - s); // Tilted direction

        rr = Ray(s, d); // Tilted ray
    }

    if (Scene::debugCurrent >= Scene::debugBegin && Scene::debugCurrent <= Scene::debugEnd)
    {
        std::cout << "Sampled pixel sample pos: " << id << " " << samplePos.first << " " << samplePos.second << " t: " << rr.time << "\n";
    }
    
    return rr; // Return the ray object
}

// Generate a pixel sample 
Pixel Camera::GenerateSampleForPixel(int row, int col) const
{
    float horz = (imgPlane.right - imgPlane.left) / imgPlane.nx; // Horz offset
    float vert = (imgPlane.top - imgPlane.bottom) / imgPlane.ny; // Vert offset

    BoundingVolume2f vol(Vector2f(0.0f, 0.0f), Vector2f(1.0f, 1.0f)); // Normalized screen volume

    return Pixel{row, col, vol, horz, vert};
}


// Given different camera construct the image plane and essential attributes for the camera
// using given values
void Camera::DeriveBoundaries(float fov, float a, const Vector3f& gazePoint, const Vector3f& pos, float d, float &l, float &r, float &b, float &t, Vector3f &camGaze)
{
    camGaze = Normalize(gazePoint - pos); // Derive camera gaze

    // Given FoV derive the image plane
    t = d * tan((fov / 2) * (3.14f / 180.0f));
    b = -t;

    r = a * t;
    l = -r;
    // --
}

}