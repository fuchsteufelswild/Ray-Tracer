#include "Light.h"
#include "Material.h"
#include "Camera.h"
#include "Image.h"
#include "Scene.h"

// #include "Shape.h"
#include "Sphere.h"
#include "Triangle.h"
#include "Mesh.h"
#include "Primitive.h"
#include "BVHTree.h"
#include "tinyxml2.h"
#include "happly.h"
#include "Texture.h"
#include "Tonemapper.h"
#include "brdf.h"
// #include <future>
#include <thread>
#include <string>
#include <chrono>
#include <memory>
#include <cmath>

using namespace tinyxml2;

namespace actracer {

Scene *Scene::pScene = nullptr; // Initialize with nullptr
int Primitive::id = 0;
int Scene::debugBegin = -1;
int Scene::debugEnd = -1;
int Scene::debugCurrent = 0;

Scene::Scene()
{
    tmo = nullptr;

    sceneRandom = Random<double>{};

    Scene::pScene = this;

    maxRecursionDepth = 0; // Default recursion depth
    shadowRayEps = 0.005;  // Default shadow ray epsilon
    intTestEps = 0.0001;
}


// Returns fresnel value for the given intersection point and a ray
// Side-effects are the change of the surface normal and flag that
// indicates if the ray is coming from inside to outside
// Correlation between the ray and the normal also changes
// Returns the new n1, n2 values
std::pair<float, float> ComputeFresnel(const SurfaceIntersection& intersection, const Ray& r, bool& flag, Vector3f& sn, float& corr, float& Fr)
{
    float n1 = r.currMat->rIndex;        // Index of outside mat
    float n2 = intersection.mat->rIndex; // Index of inside mat

    if (intersection.shape && r.currShape && intersection.shape == r.currShape) // If the ray is inside
    {
        flag = true;
        n2 = 1;
    }

    corr = Dot(r.d, sn);
    corr = Clamp<float>(corr, -1, 1);

    if (corr > 0) // Invert the normal for internal refraction
    {
        if (intersection.mat->mType == Material::MatType::CONDUCTOR)
            std::swap(n1, n2);
        sn = -sn;
    }
    else
        corr = -corr;

    float sin1 = std::sqrt(1 - corr * corr);
    float sin2 = (n1 * sin1) / n2;
    float cos2 = std::sqrt(1 - sin2 * sin2);
    Fr = intersection.mat->ComputeFresnelEffect(n1, n2, corr, cos2);
    return std::make_pair(n1, n2);
}

// Calculates the refraction color and returns if the ray is intersecting from inside to outside
bool SetRefraction(const SurfaceIntersection &intersection, const Ray &r, bool &flag, Vector3f &sn, float &Fr, Vector3f& tiltedRay)
{
    float corr;
    std::pair<float, float> tee = ComputeFresnel(intersection, r, flag, sn, corr, Fr);

    float n1 = tee.first;
    float n2 = tee.second;
    float param = n1 / n2;
    float cost = 1 - corr * corr;
    float det = 1 - param * param * cost;

    // Check if we can pass through the media
    if (det >= 0)
    {
        tiltedRay = Normalize((r.d + sn * corr) * (param)-sn * std::sqrt(det));
        return true;
    }

    return false;
}

// Compute the power absorption for given ray and intersection point
Vector3f DielectricPowerAbsorption(const Ray& r, const Intersection& intersection)
{
    Vector3f ac = intersection.mat->ACR(); // Retrieve material AC
    float distt = r(intersection.ip); // Distance
    float coeff = 0.01f * distt; // Absorption coeff
    float ecoeff = std::exp(-coeff);
    Vector3f param = {std::exp(-ac.x * distt), std::exp(-ac.y * distt), std::exp(-ac.z * distt)};

    return param;
}

// Get the tilted glossy reflection ray
void Scene::ComputeTiltedGlossyReflectionRay(Vector3f& vrd, const Material* intersectionMat)
{
    int mIndex = MinAbsElementIndex(vrd); // Get minimum absolute index

    // Create an orthonormal basis using vrd
    Vector3f rprime = vrd;
    rprime[mIndex] = 1.0f;
    rprime = Normalize(rprime);

    Vector3f u = Normalize(Cross(vrd, rprime));
    Vector3f v = Normalize(Cross(vrd, u));
    // --

    // Tilt randomly to sides
    Vector3f r1 = static_cast<float>(sceneRandom(-0.5, 0.5)) * u;
    Vector3f r2 = static_cast<float>(sceneRandom(-0.5, 0.5)) * v;
    // --

    // Compute the resulting reflection ray
    Vector3f resulting = Normalize(vrd + intersectionMat->roughness * (r1 + r2));

    vrd = resulting;
}

// Calculate light for the given ray
void Scene::CalculateLight(Ray &r, Vector3f &outColor, int depth, float ctw, float rth)
{
    // SurfaceIntersection intersection{};
    // // accelerator->Intersect(accelerator->root, r, intersection); // Find the closest object in the path

    // if (!intersection.mat) // No object in the path
    // {
    //     outColor = backgroundColor;
    //     if(bgTexture)
    //     {
    //         outColor = bgTexture->RetrieveRGBFromUV(ctw, rth);
    //     }
    //     else
    //         outColor = backgroundColor; // Return the background
    //     return;
    // }

    // if(!(intersection.shape && r.currShape && intersection.shape == r.currShape)) // If it is not an internal reflection
    //     outColor = ambientLight * intersection.mat->ARC; // Add ambient light to object colour

    // Vector3f viewerDirection = Normalize(r.o - intersection.ip); // Camera look direction
    

    // if (intersection.tex2 != nullptr)
    // {

    //     if(intersection.tex2->decalMode == Texture::DecalMode::REPLACE_NORMAL)
    //     {
    //         Vector3f normal = intersection.tex2->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y) / 255.0f;

    //         normal = Normalize((normal - 0.5f) * 2.0f);

    //         intersection.n = intersection.shape->RegulateNormal(normal, intersection);
    //         ;
    //     }
    //     else if(intersection.tex2->decalMode == Texture::DecalMode::BUMP_NORMAL)
    //     {
    //         if(intersection.tex2->texType == Texture::TextureType::PERLIN)
    //         {
    //             float E = 0.001f;
    //             Vector3f g{};
    //             g.x = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x + E, intersection.ip.y, intersection.ip.z) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;
    //             g.y = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y + E, intersection.ip.z) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;
    //             g.z = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z + E) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;


    //             Vector3f gp = Dot(g, intersection.n) * intersection.n;
    //             Vector3f gn = g - gp;

    //             intersection.n = intersection.n - gn * intersection.tex2->bumpFactor;

    //             intersection.n = Normalize(intersection.n);
    //         }
    //         else
    //         {
    //             Vector3f newNorm = intersection.shape->GetBumpedNormal(intersection.tex2, intersection, r);
    //             intersection.n = Normalize(newNorm);
    //             ;
    //         }
            
    //     }
    //     // intersection.n
    // }

    // if (intersection.tex1 && intersection.tex1->decalMode == Texture::DecalMode::REPLACE_ALL)
    // {
    //     outColor = intersection.tex1->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);

    //     return;
    // }

    // for (Light *light : lights) // For each point light
    // {

    //     if(intersection.shape && r.currShape && intersection.shape == r.currShape) // If internal reflection
    //         break;

    //     Vector3f pointToLight = light->GetLightPosition() - intersection.ip; // Light direction

    //     float distanceToLight = SqLength(pointToLight); // Distance to light

    //     pointToLight = Normalize(pointToLight); // Normalize for the ray direction

    //     if(light->lType == Light::LightType::DIRECTIONAL)
    //     {
    //         distanceToLight = 1e9;
    //         pointToLight = -light->GetLightDirection();
    //         pointToLight = Normalize(pointToLight);
    //     }
    //     else if(light->lType == Light::LightType::AREA)
    //     {
    //         Vector3f tempPos = light->GetLightDirection();
    //         pointToLight = tempPos - intersection.ip;

    //         distanceToLight = SqLength(pointToLight);
    //         pointToLight = Normalize(pointToLight);
    //     }
    //     else if(light->lType == Light::LightType::ENVIRONMENT)
    //     {
    //         Vector3f tempPos = light->GetLightDirection(intersection.n);
    //         pointToLight = tempPos;

    //         distanceToLight = SqLength(pointToLight);
    //         pointToLight = Normalize(pointToLight);
    //     }

    //     // Closest object between the intersection point and light
    //     SurfaceIntersection closestObjectInPath{};

    //     Ray tempRay = Ray(intersection.ip + intersection.n * shadowRayEps, pointToLight, nullptr, nullptr, r.time);

    //     // accelerator->Intersect(accelerator->root, tempRay, closestObjectInPath); // Find closest object in the path
    //     if (closestObjectInPath.mat) // If found                                                                                                                                                                // Closest object in the path of the ray from intersection point to light exists
    //     {
    //         float distanceToObject = SqLength(closestObjectInPath.ip - intersection.ip); // Distance to closest found object between intersection point and the light

    //         if (distanceToObject > shadowRayEps && distanceToObject < distanceToLight - shadowRayEps) // Object is closer than light ( it is in-between )
    //             continue;
    //     }
        
    //     if (intersection.mat->brdf)
    //     {
    //         Vector3f wi = pointToLight;
    //         Vector3f wo = viewerDirection;
    //         Vector3f kd = intersection.mat->DRC;
    //         Vector3f ks = intersection.mat->SRC;
    //         Vector3f surfaceNormal = intersection.n;

    //         Vector3f brdfValue = intersection.mat->brdf->f(wi, wo, surfaceNormal, kd, ks, intersection.mat->rIndex);
    //         outColor += light->GetLightIntensity() / (distanceToLight * distanceToLight) * brdfValue;
    //     }
    //     else if(tmo)
    //         outColor += light->ResultingColorContribution(intersection, viewerDirection, intersection.tex1, tmo->GetGamma()); // Add the light contribution
    //     else
    //         outColor += light->ResultingColorContribution(intersection, viewerDirection, intersection.tex1); // Add the light contribution

        

    // }

    // // Recursive tracing
    // if(depth < maxRecursionDepth && intersection.mat->mType != Material::MatType::DEFAULT)
    // {
    //     Vector3f sn = Normalize(intersection.n); // Surface normal
    //     float Fr = 1;
    //     bool flag = false;
    //     if (intersection.mat->mType != Material::MatType::MIRROR)
    //     {
    //         Vector3f tiltedRay{};

    //         if(SetRefraction(intersection, r, flag, sn, Fr, tiltedRay)) // Get the tilted ray
    //         {
    //             Vector3f fracColor{};
    //             if (!flag) // Ray is coming from outside
    //             {
    //                 Ray tempRay = Ray(intersection.ip + -sn * shadowRayEps, tiltedRay, intersection.mat, intersection.shape, r.time);                  
    //                 CalculateLight(tempRay, fracColor, depth + 1);
    //             }
    //             else // Ray is inside
    //             {
    //                 Ray tempRay = Ray(intersection.ip + -sn * shadowRayEps, tiltedRay, defMat, nullptr, r.time);
    //                 CalculateLight(tempRay, fracColor, depth + 1);
    //             }
    //             if (!(fracColor.x == backgroundColor.x && fracColor.y == backgroundColor.y && fracColor.z == backgroundColor.z)) // If it Intersects with an object
    //                 outColor = outColor + fracColor * (1 - Fr);
    //         }
    //         else if (intersection.shape == r.currShape) // Did not change
    //             Fr = 1;
    //     }

    //     if(intersection.mat->mType == Material::MatType::MIRROR || intersection.mat->mType == Material::MatType::CONDUCTOR)
    //     {

    //         if (intersection.mat->MRC.x != 0 || intersection.mat->MRC.y != 0 || intersection.mat->MRC.z != 0)
    //         {
    //             float gam = 1.0f;
    //             if(intersection.mat->degamma)
    //                 gam = 2.2f;

    //             Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
    //             Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f);                                     // Resulting mirror color

    //             // If roughness > 0 apply glossy reflection
    //             if (intersection.mat->roughness > 0)
    //                 ComputeTiltedGlossyReflectionRay(vrd, intersection.mat);

    //             Ray tempRay = Ray(intersection.ip + sn * shadowRayEps, vrd, r.currMat, r.currShape, r.time);
    //             CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

    //             if (!(mirCol.x == backgroundColor.x && mirCol.y == backgroundColor.y && mirCol.z == backgroundColor.z) && intersection.mat->mType == Material::MatType::MIRROR)
    //                 outColor = outColor + mirCol * intersection.mat->MRC * gam;
    //             if (!(mirCol.x == backgroundColor.x && mirCol.y == backgroundColor.y && mirCol.z == backgroundColor.z) && intersection.mat->mType == Material::MatType::CONDUCTOR) // If it Intersects with an object
    //                 outColor = outColor + mirCol * intersection.mat->MRC * Fr;
    //         }
    //     }

    //     if(intersection.mat->mType == Material::MatType::DIELECTRIC)
    //     {
    //         Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
    //         Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f); // Resulting mirror color

    //         Ray tempRay = Ray(intersection.ip + sn * shadowRayEps, vrd, r.currMat, r.currShape, r.time);
    //         CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

    //         if (intersection.mat->mType == Material::MatType::DIELECTRIC)
    //         {
    //             outColor = outColor + mirCol * Fr;

    //             if (flag) // If ray was inside
    //             {
    //                 Vector3f param = DielectricPowerAbsorption(r, intersection);
    //                 outColor *= param;
    //             }
    //         }
    //     }
    // }
}

Color ObtainColor(const Vector3f &v)
{
    unsigned char x, y, z;

    // Clamp the values between 0 - 255
    x = v.x > 255.0f ? 255 : v.x;
    y = v.y > 255.0f ? 255 : v.y;
    z = v.z > 255.0f ? 255 : v.z;
    // --

    return {x, y, z};
}

Color Scene::RenderPixel(Camera* cam, int row, int col)
{
    Vector3f resultingColor{};
    Pixel px = cam->GenerateSampleForPixel(row, col);
    
    Random<double> timeRandom{};

    Vector3f sum{};


    for(int i = 0; i < cam->GetSampleCount(); ++i)
    {
        PixelSample pxs;

        Ray r = cam->GenerateRayFromPixelSample(px, i, pxs);

        r.currMat = defMat;
        r.time = timeRandom(0.0, 1.0);
        Vector3f res;

        CalculateLight(r, res, 0, (float)col / cam->imgPlane.nx, (float)row / cam->imgPlane.ny);

        sum += res;
    }
    
    
    sum /= cam->GetSampleCount();

    if (tmo)
        inVector.push_back(sum);

    return ObtainColor(sum);
}

// Find the out color for the given ray
Color Scene::RenderPixel(Ray &ray, float ctw, float rth)
{
    Vector3f out; // All light contributions
    CalculateLight(ray, out, 0, ctw, rth); // Fill the colour output

    unsigned char xx, yy, z; // Returning colour values

    // Clamp the values between 0 - 255
    xx = out.x > 255.0f ? 255 : out.x;
    yy = out.y > 255.0f ? 255 : out.y;
    z = out.z > 255.0f ? 255 : out.z;
    // --
    if(tmo)
        inVector.push_back(out);

    return {xx, yy, z};
}

// Render Camera pixels in the interval [iStart - iEnd)
void Scene::RenderCam(Camera *cam, Image &img, int iStart, int iEnd)
{
    for (int i = iStart; i < iEnd; ++i)
    {
        for (int j = 0; j < cam->imgPlane.nx; ++j)
        {
            Scene::debugCurrent = j;

            Color res = Color(0, 0, 0);
            if(cam->GetSampleCount() == 1)
            {
                Ray r = cam->GenerateRay(i, j); // Get the ray
                
                int nextI = i + 1;
                int nextJ = j + 1;
                if(nextI >= iEnd)
                    nextI = nextI - 1;
                if(nextJ >= cam->imgPlane.nx)
                    nextJ = cam->imgPlane.nx - 1;
                
                    
                r.currMat = defMat;
                res = RenderPixel(r, (float)j / cam->imgPlane.nx, (float)i / cam->imgPlane.ny); // Find the colour of the pixel
            }
            else
                res = RenderPixel(cam, i, j);

            img.SetPixelColor(j, i, res);   // Set pixel colour in the image
        }
    }

    //system("pause");
}

// For each camera render the scene from its point of view
// 8 threads are used by partitioning rows into 8 chunks
// and filling the pixels in these chunks simultenously
void Scene::RenderScene(void)
{
    defMat = new Material{};
    

    for (Camera *cam : cameras) // For each camera in the scene
    {

        auto start = std::chrono::high_resolution_clock::now(); // Start recording time

        Image img(cam->imgPlane.nx, cam->imgPlane.ny); // Create an image
        int rowDiff = cam->imgPlane.ny / 8;            // Get row difference between successive chunks

        RenderCam(cam, img, 0, cam->imgPlane.ny);

        // std::thread th1(&Scene::RenderCam, this, cam, std::ref(img), 0, cam->imgPlane.ny);
        // th1.join();
        // std::thread th1(&Scene::RenderCam, this, cam, std::ref(img), 0, rowDiff);
        // std::thread th2(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 1, rowDiff * 2);
        // std::thread th3(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 2, rowDiff * 3);
        // std::thread th4(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 3, rowDiff * 4);
        // std::thread th5(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 4, rowDiff * 5);
        // std::thread th6(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 5, rowDiff * 6);
        // std::thread th7(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 6, rowDiff * 7);
        // std::thread th8(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 7, cam->imgPlane.ny);
        
        // th1.join(); th2.join(); th3.join(); th4.join(); th5.join(); th6.join(); th7.join(); th8.join();
        if(tmo)
        {
            // std::cout << "TONEMAPPING EXISTS\n";

            float* f = tmo->Tonemap(img);
            tmo->SaveEXR(f, img.GetImageWidth(), img.GetImageHeight(), cam->imageName);
        }
        else
        {
            img.SaveImageAsPPM(cam->imageName); // Save the resulting image
        }
        
        inVector.erase(inVector.begin(), inVector.end());
        inVector.clear();

        auto end = std::chrono::high_resolution_clock::now(); // Rendering finished for the camera

        std::cout << "Camera: " << cam->GetID() << " finished rendering in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f << "ms\n";
    }
}

void Scene::ComputeTransformMatrix(const char *ch, Transform &objTransform)
{
    std::cout << "New Transform: \n";
    while (*ch != '\0')
    {
        switch (*ch)
        {
        case 'r':
        {
            int tid = *(ch + 1) - 48;
            std::cout << "r" << tid << "\n";
            for (auto itr = rotations.begin(); itr != rotations.end(); ++itr)
            {
                if ((*itr)->id == tid)
                {
                    objTransform = objTransform(*(*itr));
                    break;
                }
            }
        }
        break;
        case 't':
        {
            
            int tid = *(ch + 1) - 48;
            std::cout << "t" << tid << "\n";
            for (auto itr = translations.begin(); itr != translations.end(); ++itr)
            {
                if ((*itr)->id == tid)
                {
                    objTransform = objTransform(*(*itr));
                    break;
                }
            }
        }
        break;
        case 's':
        {
            int tid = *(ch + 1) - 48;
            std::cout << "s" << tid << "\n";
            for (auto itr = scalings.begin(); itr != scalings.end(); ++itr)
            {
                if ((*itr)->id == tid)
                {
                    objTransform = objTransform(*(*itr));
                    break;
                }
            }
        }
        break;
        default:
            break;
        }

        ++ch;
    }
}

Shape* Scene::GetMeshWithID(int id, Shape::ShapeType shapeType)
{
    for(Shape* sh : objects)
        if(sh->shType == shapeType && id == sh->id)
            return sh;
}


}
