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
#include "Acceleration.h"
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

std::vector<Camera*>& Scene::GetAllCameras()
{
    return cameras;
}

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
    SurfaceIntersection intersection{};
    accelerator->Intersect(accelerator->root, r, intersection); // Find the closest object in the path

    if (!intersection.mat) // No object in the path
    {
        outColor = backgroundColor;
        if(bgTexture)
        {
            outColor = bgTexture->RetrieveRGBFromUV(ctw, rth);
        }
        else
            outColor = backgroundColor; // Return the background
        return;
    }

    if(!(intersection.shape && r.currShape && intersection.shape == r.currShape)) // If it is not an internal reflection
        outColor = ambientLight * intersection.mat->ARC; // Add ambient light to object colour

    Vector3f viewerDirection = Normalize(r.o - intersection.ip); // Camera look direction
    

    if (intersection.tex2 != nullptr)
    {

        if(intersection.tex2->decalMode == Texture::DecalMode::REPLACE_NORMAL)
        {
            Vector3f normal = intersection.tex2->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y) / 255.0f;

            normal = Normalize((normal - 0.5f) * 2.0f);

            intersection.n = intersection.shape->RegulateNormal(normal, intersection);
            ;
        }
        else if(intersection.tex2->decalMode == Texture::DecalMode::BUMP_NORMAL)
        {
            if(intersection.tex2->texType == Texture::TextureType::PERLIN)
            {
                float E = 0.001f;
                Vector3f g{};
                g.x = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x + E, intersection.ip.y, intersection.ip.z) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;
                g.y = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y + E, intersection.ip.z) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;
                g.z = (intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z + E) - intersection.tex2->RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z)).x / E;


                Vector3f gp = Dot(g, intersection.n) * intersection.n;
                Vector3f gn = g - gp;

                intersection.n = intersection.n - gn * intersection.tex2->bumpFactor;

                intersection.n = Normalize(intersection.n);
            }
            else
            {
                Vector3f newNorm = intersection.shape->GetBumpedNormal(intersection.tex2, intersection, r);
                intersection.n = Normalize(newNorm);
                ;
            }
            
        }
        // intersection.n
    }

    if (intersection.tex1 && intersection.tex1->decalMode == Texture::DecalMode::REPLACE_ALL)
    {
        outColor = intersection.tex1->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);

        return;
    }

    for (Light *light : lights) // For each point light
    {

        if(intersection.shape && r.currShape && intersection.shape == r.currShape) // If internal reflection
            break;

        Vector3f pointToLight = light->GetLightPosition() - intersection.ip; // Light direction

        float distanceToLight = SqLength(pointToLight); // Distance to light

        pointToLight = Normalize(pointToLight); // Normalize for the ray direction

        if(light->lType == Light::LightType::DIRECTIONAL)
        {
            distanceToLight = 1e9;
            pointToLight = -light->GetLightDirection();
            pointToLight = Normalize(pointToLight);
        }
        else if(light->lType == Light::LightType::AREA)
        {
            Vector3f tempPos = light->GetLightDirection();
            pointToLight = tempPos - intersection.ip;

            distanceToLight = SqLength(pointToLight);
            pointToLight = Normalize(pointToLight);
        }
        else if(light->lType == Light::LightType::ENVIRONMENT)
        {
            Vector3f tempPos = light->GetLightDirection(intersection.n);
            pointToLight = tempPos;

            distanceToLight = SqLength(pointToLight);
            pointToLight = Normalize(pointToLight);
        }

        // Closest object between the intersection point and light
        SurfaceIntersection closestObjectInPath{};

        Ray tempRay = Ray(intersection.ip + intersection.n * shadowRayEps, pointToLight, nullptr, nullptr, r.time);

        accelerator->Intersect(accelerator->root, tempRay, closestObjectInPath); // Find closest object in the path
        if (closestObjectInPath.mat) // If found                                                                                                                                                                // Closest object in the path of the ray from intersection point to light exists
        {
            float distanceToObject = SqLength(closestObjectInPath.ip - intersection.ip); // Distance to closest found object between intersection point and the light

            if (distanceToObject > shadowRayEps && distanceToObject < distanceToLight - shadowRayEps) // Object is closer than light ( it is in-between )
                continue;
        }
        
        if (intersection.mat->brdf)
        {
            Vector3f wi = pointToLight;
            Vector3f wo = viewerDirection;
            Vector3f kd = intersection.mat->DRC;
            Vector3f ks = intersection.mat->SRC;
            Vector3f surfaceNormal = intersection.n;

            Vector3f brdfValue = intersection.mat->brdf->f(wi, wo, surfaceNormal, kd, ks, intersection.mat->rIndex);
            outColor += light->GetLightIntensity() / (distanceToLight * distanceToLight) * brdfValue;
        }
        else if(tmo)
            outColor += light->ResultingColorContribution(intersection, viewerDirection, intersection.tex1, tmo->GetGamma()); // Add the light contribution
        else
            outColor += light->ResultingColorContribution(intersection, viewerDirection, intersection.tex1); // Add the light contribution

        

    }

    // Recursive tracing
    if(depth < maxRecursionDepth && intersection.mat->mType != Material::MatType::DEFAULT)
    {
        Vector3f sn = Normalize(intersection.n); // Surface normal
        float Fr = 1;
        bool flag = false;
        if (intersection.mat->mType != Material::MatType::MIRROR)
        {
            Vector3f tiltedRay{};

            if(SetRefraction(intersection, r, flag, sn, Fr, tiltedRay)) // Get the tilted ray
            {
                Vector3f fracColor{};
                if (!flag) // Ray is coming from outside
                {
                    Ray tempRay = Ray(intersection.ip + -sn * shadowRayEps, tiltedRay, intersection.mat, intersection.shape, r.time);                  
                    CalculateLight(tempRay, fracColor, depth + 1);
                }
                else // Ray is inside
                {
                    Ray tempRay = Ray(intersection.ip + -sn * shadowRayEps, tiltedRay, defMat, nullptr, r.time);
                    CalculateLight(tempRay, fracColor, depth + 1);
                }
                if (!(fracColor.x == backgroundColor.x && fracColor.y == backgroundColor.y && fracColor.z == backgroundColor.z)) // If it Intersects with an object
                    outColor = outColor + fracColor * (1 - Fr);
            }
            else if (intersection.shape == r.currShape) // Did not change
                Fr = 1;
        }

        if(intersection.mat->mType == Material::MatType::MIRROR || intersection.mat->mType == Material::MatType::CONDUCTOR)
        {

            if (intersection.mat->MRC.x != 0 || intersection.mat->MRC.y != 0 || intersection.mat->MRC.z != 0)
            {
                float gam = 1.0f;
                if(intersection.mat->degamma)
                    gam = 2.2f;

                Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
                Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f);                                     // Resulting mirror color

                // If roughness > 0 apply glossy reflection
                if (intersection.mat->roughness > 0)
                    ComputeTiltedGlossyReflectionRay(vrd, intersection.mat);

                Ray tempRay = Ray(intersection.ip + sn * shadowRayEps, vrd, r.currMat, r.currShape, r.time);
                CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

                if (!(mirCol.x == backgroundColor.x && mirCol.y == backgroundColor.y && mirCol.z == backgroundColor.z) && intersection.mat->mType == Material::MatType::MIRROR)
                    outColor = outColor + mirCol * intersection.mat->MRC * gam;
                if (!(mirCol.x == backgroundColor.x && mirCol.y == backgroundColor.y && mirCol.z == backgroundColor.z) && intersection.mat->mType == Material::MatType::CONDUCTOR) // If it Intersects with an object
                    outColor = outColor + mirCol * intersection.mat->MRC * Fr;
            }
        }

        if(intersection.mat->mType == Material::MatType::DIELECTRIC)
        {
            Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
            Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f); // Resulting mirror color

            Ray tempRay = Ray(intersection.ip + sn * shadowRayEps, vrd, r.currMat, r.currShape, r.time);
            CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

            if (intersection.mat->mType == Material::MatType::DIELECTRIC)
            {
                outColor = outColor + mirCol * Fr;

                if (flag) // If ray was inside
                {
                    Vector3f param = DielectricPowerAbsorption(r, intersection);
                    outColor *= param;
                }
            }
        }
    }
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

            img.SetPixelValue(j, i, res);   // Set pixel colour in the image
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

// Parses XML file
Scene::Scene(const char *xmlPath)
{
    tmo = nullptr;

    sceneRandom = Random<double>{};

    seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator = std::default_random_engine(seed);
    distribution = std::uniform_real_distribution<double>(-0.5, 0.5);

    Scene::pScene = this;

    const char *str;
    XMLDocument xmlDoc;
    XMLError eResult;
    XMLElement *pElement;

    maxRecursionDepth = 0; // Default recursion depth
    shadowRayEps = 0.005;  // Default shadow ray epsilon
    intTestEps = 0.0001;
    

    eResult = xmlDoc.LoadFile(xmlPath);

    XMLNode *pRoot = xmlDoc.FirstChild();

    // Recursion depth
    pElement = pRoot->FirstChildElement("MaxRecursionDepth");
    if (pElement != nullptr)
        pElement->QueryIntText(&maxRecursionDepth);

    // Background color
    pElement = pRoot->FirstChildElement("BackgroundColor");
    if(pElement != nullptr)
    {
        str = pElement->GetText();
        sscanf(str, "%f %f %f", &backgroundColor.r, &backgroundColor.g, &backgroundColor.b);
    }
    // Shadow epsilon
    pElement = pRoot->FirstChildElement("ShadowRayEpsilon");
    if (pElement != nullptr)
        pElement->QueryFloatText(&shadowRayEps);

    // Intersection epsilon
    pElement = pRoot->FirstChildElement("intersectionTestEpsilon");
    if (pElement != nullptr)
        eResult = pElement->QueryFloatText(&intTestEps);

    // Parse cameras
    pElement = pRoot->FirstChildElement("Cameras");
    XMLElement *pCamera = pElement->FirstChildElement("Camera");
    XMLElement *camElement;
    while (pCamera != nullptr)
    {
        int id;
        int numSamples = 1;
        char imageName[64];
        Vector3f pos, gaze, up;
        ImagePlane imgPlane;
        float focalDistance = 0.0f;
        float apertureSize = 0.0f;
        int mode = 0;  // Default mode is projective
        int iType = 0; // Default input type is ( l, r, b, t, d )

        eResult = pCamera->QueryIntAttribute("id", &id); // Cam id

        const char *attr = pCamera->Attribute("type");

        if(attr != nullptr)
            if(strcmp(attr, "lookAt") == 0)
                iType = 1;

        // Parse the common attributes ( they will exist in every xml file )
        camElement = pCamera->FirstChildElement("Position");
        str = camElement->GetText();
        sscanf(str, "%f %f %f", &pos.x, &pos.y, &pos.z);

        camElement = pCamera->FirstChildElement("Up");
        str = camElement->GetText();
        sscanf(str, "%f %f %f", &up.x, &up.y, &up.z);

        camElement = pCamera->FirstChildElement("NumSamples");
        if(camElement != nullptr)
            eResult = camElement->QueryIntText(&numSamples);

        camElement = pCamera->FirstChildElement("FocusDistance");
        if (camElement != nullptr)
            eResult = camElement->QueryFloatText(&focalDistance);

        camElement = pCamera->FirstChildElement("ApertureSize");
        if (camElement != nullptr)
            eResult = camElement->QueryFloatText(&apertureSize);

        camElement = pCamera->FirstChildElement("NearDistance");
        eResult = camElement->QueryFloatText(&imgPlane.distance);
        camElement = pCamera->FirstChildElement("ImageResolution");
        str = camElement->GetText();
        sscanf(str, "%d %d", &imgPlane.nx, &imgPlane.ny);
        camElement = pCamera->FirstChildElement("ImageName");
        str = camElement->GetText();
        strcpy(imageName, str);

        if(!iType) // Use directly given boundaries
		{
            camElement = pCamera->FirstChildElement("Gaze");
            str = camElement->GetText();
            sscanf(str, "%f %f %f", &gaze.x, &gaze.y, &gaze.z);

            camElement = pCamera->FirstChildElement("NearPlane");
			str = camElement->GetText();
			sscanf(str, "%f %f %f %f", &imgPlane.left, &imgPlane.right, &imgPlane.bottom, &imgPlane.top);
		}
		else // Use fov and aspect ratio to derive the boundaries
		{
            float fovY = -1; // Field of view of the frustum planes on sides
            Vector3f gazePoint; // The point the camera is looking at

            camElement = pCamera->FirstChildElement("GazePoint");
            str = camElement->GetText();
            sscanf(str, "%f %f %f", &gazePoint.x, &gazePoint.y, &gazePoint.z);

            camElement = pCamera->FirstChildElement("FovY");
            eResult = camElement->QueryFloatText(&fovY);

			// Derive (l, r, b, t)
			Camera::DeriveBoundaries(fovY, static_cast<float>(static_cast<float>(imgPlane.nx) / imgPlane.ny), gazePoint, pos,
                                     imgPlane.distance, imgPlane.left, imgPlane.right, imgPlane.bottom, imgPlane.top, gaze); 

		}

        camElement = pCamera->FirstChildElement("Tonemap");
        if(camElement != nullptr)
        {
            std::string type;
            float key, burn;
            float saturation;
            float gamma;

            XMLElement* toneElement;
            toneElement = camElement->FirstChildElement("TMOOptions");
            str = toneElement->GetText();
            sscanf(str, "%f %f", &key, &burn);

            toneElement = camElement->FirstChildElement("Saturation");
            toneElement->QueryFloatText(&saturation);
            toneElement = camElement->FirstChildElement("Gamma");
            toneElement->QueryFloatText(&gamma);

            tmo = new Tonemapper(key, burn, saturation, gamma);
        }

        cameras.push_back(new Camera(id, imageName, pos, gaze, up, imgPlane, numSamples, focalDistance, apertureSize));

        pCamera = pCamera->NextSiblingElement("Camera");
    }

    // Parse BRDFs
    pElement = pRoot->FirstChildElement("BRDFs");

    if(pElement != nullptr)
    {
        XMLElement *pBRDF = pElement->FirstChildElement("ModifiedBlinnPhong");
        XMLElement *pBRDFElem;
        int _id;

        while (pBRDF != nullptr)
        {
            float _phong = 1;
            bool _normalized = false;

            pBRDF->QueryIntAttribute("id", &_id);
            pBRDF->QueryBoolAttribute("normalized", &_normalized);
            pBRDFElem = pBRDF->FirstChildElement("Exponent");
            if (pBRDFElem != nullptr)
                pBRDFElem->QueryFloatText(&_phong);

            brdfs.push_back(new BRDFBlinnPhongModified(_phong, _normalized));

            pBRDF = pBRDF->NextSiblingElement("ModifiedBlinnPhong");
        }

        pBRDF = pElement->FirstChildElement("OriginalPhong");
        while (pBRDF != nullptr)
        {
            float _phong = 1;

            pBRDF->QueryIntAttribute("id", &_id);

            pBRDFElem = pBRDF->FirstChildElement("Exponent");
            if (pBRDFElem != nullptr)
                pBRDFElem->QueryFloatText(&_phong);

            brdfs.push_back(new BRDFPhongOriginal(_phong));

            pBRDF = pBRDF->NextSiblingElement("OriginalPhong");
        }

        pBRDF = pElement->FirstChildElement("OriginalBlinnPhong");
        while (pBRDF != nullptr)
        {
            float _phong = 1;

            pBRDF->QueryIntAttribute("id", &_id);

            pBRDFElem = pBRDF->FirstChildElement("Exponent");
            if (pBRDFElem != nullptr)
                pBRDFElem->QueryFloatText(&_phong);

            brdfs.push_back(new BRDFBlinnPhongOriginal(_phong));

            pBRDF = pBRDF->NextSiblingElement("OriginalBlinnPhong");
        }

        pBRDF = pElement->FirstChildElement("ModifiedPhong");
        while (pBRDF != nullptr)
        {
            float _phong = 1;
            bool _normalized = false;

            pBRDF->QueryIntAttribute("id", &_id);
            pBRDF->QueryBoolAttribute("normalized", &_normalized);
            pBRDFElem = pBRDF->FirstChildElement("Exponent");
            if (pBRDFElem != nullptr)
                pBRDFElem->QueryFloatText(&_phong);

            brdfs.push_back(new BRDFPhongModified(_phong, _normalized));

            pBRDF = pBRDF->NextSiblingElement("ModifiedPhong");
        }

        pBRDF = pElement->FirstChildElement("TorranceSparrow");
        while (pBRDF != nullptr)
        {
            float _phong = 1;
            bool _kdfresnel = false;

            pBRDF->QueryIntAttribute("id", &_id);
            pBRDF->QueryBoolAttribute("kdfresnel", &_kdfresnel);
            pBRDFElem = pBRDF->FirstChildElement("Exponent");
            if (pBRDFElem != nullptr)
                pBRDFElem->QueryFloatText(&_phong);

            brdfs.push_back(new BRDFTorranceSparrow(_phong, _kdfresnel));

            pBRDF = pBRDF->NextSiblingElement("TorranceSparrow");
        }
    }
    
    // Parse materials
    pElement = pRoot->FirstChildElement("Materials");
    XMLElement *pMaterial = pElement->FirstChildElement("Material");
    XMLElement *materialElement;
    while (pMaterial != nullptr)
    {
        bool degamma = false;
        int curr = materials.size() - 1;
        
        Vector3f _ARC;
        Vector3f _DRC;
        Vector3f _SRC;
        Vector3f _MRC;
        Vector3f _AC;
        float    _AI;
        float    _RI;
        float    _roughness;
        int      _phong = 1;
        int      _id;
        int      _brdfID = -1;
        BRDFBase* matBRDF = nullptr;


        Material::MatType _typ = Material::MatType::DEFAULT;

        eResult = pMaterial->QueryIntAttribute("id", &_id);
        eResult = pMaterial->QueryBoolAttribute("degamma", &degamma);
        eResult = pMaterial->QueryIntAttribute("BRDF", &_brdfID);
        const char* attr = pMaterial->Attribute("type");

        if(_brdfID != -1)
            matBRDF = brdfs[_brdfID - 1];

        if(attr != nullptr)
        {
            if(strcmp(attr, "mirror") == 0)
                _typ = Material::MatType::MIRROR;
            else if(strcmp(attr, "dielectric") == 0)
                _typ = Material::MatType::DIELECTRIC;
            else if(strcmp(attr, "conductor") == 0)
                _typ = Material::MatType::CONDUCTOR;
        }

        materialElement = pMaterial->FirstChildElement("AmbientReflectance");
        str = materialElement->GetText();
        sscanf(str, "%f %f %f", &_ARC.r, &_ARC.g, &_ARC.b);
        materialElement = pMaterial->FirstChildElement("DiffuseReflectance");
        str = materialElement->GetText();
        sscanf(str, "%f %f %f", &_DRC.r, &_DRC.g, &_DRC.b);
        materialElement = pMaterial->FirstChildElement("SpecularReflectance");
        str = materialElement->GetText();
        sscanf(str, "%f %f %f", &_SRC.r, &_SRC.g, &_SRC.b);

        materialElement = pMaterial->FirstChildElement("MirrorReflectance");
        if (materialElement != nullptr)
        {
            str = materialElement->GetText();
            sscanf(str, "%f %f %f", &_MRC.r, &_MRC.g, &_MRC.b);
        }
        else
        {
            _MRC.r = 0.0;
            _MRC.g = 0.0;
            _MRC.b = 0.0;
        }

        materialElement = pMaterial->FirstChildElement("AbsorptionCoefficient");
        if(materialElement != nullptr)
        {
            str = materialElement->GetText();
            sscanf(str, "%f %f %f", &_AC.r, &_AC.g, &_AC.b);
        }
        else
        {
            _AC.r = 0.0;
            _AC.g = 0.0;
            _AC.b = 0.0;
        }
        

        materialElement = pMaterial->FirstChildElement("RefractionIndex");
        if(materialElement != nullptr)
            materialElement->QueryFloatText(&_RI);
        else
            _RI = 1;

        materialElement = pMaterial->FirstChildElement("AbsorptionIndex");
        if (materialElement != nullptr)
            materialElement->QueryFloatText(&_AI);
        else
            _AI = 1;

        materialElement = pMaterial->FirstChildElement("PhongExponent");
        if (materialElement != nullptr)
            materialElement->QueryIntText(&_phong);
        else
            _phong = 0;

        materialElement = pMaterial->FirstChildElement("Roughness");
        if (materialElement != nullptr)
            materialElement->QueryFloatText(&_roughness);
        else
            _roughness = 0;

        switch(_typ)
        {
            case Material::MatType::CONDUCTOR:
                materials.push_back(new Conductor(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness, _AI));
                break;
            case Material::MatType::DIELECTRIC:
                materials.push_back(new Dielectric(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness, _AC));
                break;
            default:
                materials.push_back(new Material(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness));
                break;
        }

        materials.back()->degamma = degamma;
        materials.back()->brdf = matBRDF;

        if(matBRDF)
            materials.back()->brdf->ai = _AI;

        if(materials.back()->degamma)
        {
            materials.back()->DRC.x = std::pow(materials.back()->DRC.x, 2.2f);
            materials.back()->DRC.y = std::pow(materials.back()->DRC.y, 2.2f);
            materials.back()->DRC.z = std::pow(materials.back()->DRC.z, 2.2f);

            materials.back()->SRC.x = std::pow(materials.back()->SRC.x, 2.2f);
            materials.back()->SRC.y = std::pow(materials.back()->SRC.y, 2.2f);
            materials.back()->SRC.z = std::pow(materials.back()->SRC.z, 2.2f);

            materials.back()->ARC.x = std::pow(materials.back()->ARC.x, 2.2f);
            materials.back()->ARC.y = std::pow(materials.back()->ARC.y, 2.2f);
            materials.back()->ARC.z = std::pow(materials.back()->ARC.z, 2.2f);
        }

        pMaterial = pMaterial->NextSiblingElement("Material");
    }

    pElement = pRoot->FirstChildElement("Textures");

    if(pElement != nullptr)
    {
        XMLElement *textureElement = pElement->FirstChildElement("Images");
        if(textureElement != nullptr)
        {
            XMLElement* imageElement = textureElement->FirstChildElement("Image");

            while(imageElement != nullptr)
            {
                int id;
                const char* imPath;

                eResult = imageElement->QueryIntAttribute("id", &id);

                imPath = imageElement->GetText();
                if (imPath != nullptr)
                    this->imagePaths.push_back(std::string(imPath));

                imageElement = imageElement->NextSiblingElement("Image");
            }
        }
        textureElement = pElement->FirstChildElement("TextureMap");

        while (textureElement != nullptr)
        {
            XMLElement *miniElement;

            int id;

            PerlinTexture::NoiseConversionType nct = PerlinTexture::NoiseConversionType::LINEAR; // noise conversion
            ImageTexture::InterpolationType itype = ImageTexture::InterpolationType::NEAREST;
            Texture::DecalMode decalMode;
            Texture::TextureType ttype;
            float noiseScale = 1.0f;
            float bumpFactor = 1.0f;
            int normalizer = 255;
            int imageID;

            eResult = textureElement->QueryIntAttribute("id", &id);

            miniElement = textureElement->FirstChildElement("BumpFactor");
            if (miniElement != nullptr)
                miniElement->QueryFloatText(&bumpFactor);

            miniElement = textureElement->FirstChildElement("DecalMode");
            if (miniElement != nullptr)
            {
                const char *decal = miniElement->GetText();
                if (strcmp(decal, "bump_normal") == 0)
                {
                    decalMode = Texture::DecalMode::BUMP_NORMAL;
                }
                else if (strcmp(decal, "replace_kd") == 0)
                {
                    decalMode = Texture::DecalMode::REPLACE_KD;
                }
                else if (strcmp(decal, "replace_normal") == 0)
                {
                    decalMode = Texture::DecalMode::REPLACE_NORMAL;
                }
                else if (strcmp(decal, "replace_background") == 0)
                {
                    decalMode = Texture::DecalMode::REPLACE_BACKGROUND;
                }
                else if (strcmp(decal, "replace_all") == 0)
                {
                    decalMode = Texture::DecalMode::REPLACE_ALL;
                }
                else if (strcmp(decal, "blend_kd") == 0)
                {
                    decalMode = Texture::DecalMode::BLEND_KD;
                }
            }

            const char *attrType = textureElement->Attribute("type");

            if (attrType != nullptr)
            {
                if (strcmp(attrType, "perlin") == 0)
                {
                    ttype = Texture::TextureType::PERLIN;
                    XMLElement *noiseElement = textureElement->FirstChildElement("NoiseScale");
                    if (noiseElement != nullptr)
                        noiseElement->QueryFloatText(&noiseScale);

                    noiseElement = textureElement->FirstChildElement("NoiseConversion");
                    if (noiseElement != nullptr)
                    {
                        attrType = noiseElement->GetText();
                        if (attrType != nullptr)
                        {
                            if (strcmp(attrType, "absval") == 0)
                                nct = PerlinTexture::NoiseConversionType::ABSVAL;
                            else
                                nct = PerlinTexture::NoiseConversionType::LINEAR;
                        }
                    }

                    textures.push_back(new PerlinTexture(id, decalMode, bumpFactor, noiseScale, nct));
                }
                else if (strcmp(attrType, "image") == 0)
                {
                    ttype = Texture::TextureType::IMAGE;

                    XMLElement *imageElement = textureElement->FirstChildElement("ImageId");
                    if (imageElement != nullptr)
                        imageElement->QueryIntText(&imageID);

                    imageElement = textureElement->FirstChildElement("Normalizer");
                    if (imageElement != nullptr)
                        imageElement->QueryIntText(&normalizer);

                    imageElement = textureElement->FirstChildElement("Interpolation");
                    if (imageElement != nullptr)
                    {
                        attrType = imageElement->GetText();
                        if (attrType != nullptr)
                        {
                            if (strcmp(attrType, "bilinear") == 0)
                                itype == ImageTexture::InterpolationType::BILINEAR;
                            else if (strcmp(attrType, "nearest") == 0)
                                itype == ImageTexture::InterpolationType::NEAREST;
                        }
                    }

                    if(imagePaths[imageID - 1].back() == 'r')
                    {
                        TMOData out = Tonemapper::ReadExr(imagePaths[imageID - 1]);
                        textures.push_back(new ImageTexture(imagePaths[imageID - 1], id, decalMode, bumpFactor, itype, normalizer, out.data));
                        textures.back()->width = out.width;
                        textures.back()->height = out.height;
                    }
                    else
                        textures.push_back(new ImageTexture(imagePaths[imageID - 1], id, decalMode, bumpFactor, itype, normalizer));
                }
            }

            if (decalMode == Texture::DecalMode::REPLACE_BACKGROUND)
                this->bgTexture = textures.back();

            std::cout << id << " " << imageID << " " << nct << " " << itype << " " << decalMode << " " << bumpFactor << " " << normalizer << " " << noiseScale << "\n";

            textureElement = textureElement->NextSiblingElement("TextureMap");
        }
    }






    pElement = pRoot->FirstChildElement("Transformations");
    XMLElement* pTransformation;
    if(pElement != nullptr)
        pTransformation = pElement->FirstChildElement("Scaling");
    while(pTransformation != nullptr)
    {
        int id;
        float scaleX;
        float scaleY;
        float scaleZ;

        str = pTransformation->GetText();

        eResult = pTransformation->QueryIntAttribute("id", &id);
        sscanf(str, "%f %f %f", &scaleX, &scaleY, &scaleZ);

        // transformations.push_back(new Scaling(id, scaleX, scaleY, scaleZ));
        scalings.push_back(new Scaling(id, scaleX, scaleY, scaleZ));
        pTransformation = pTransformation->NextSiblingElement("Scaling");
    }

    if(pElement != nullptr)
        pTransformation = pElement->FirstChildElement("Rotation");
    while (pTransformation != nullptr)
    {
        int id;
        glm::vec3 axis;
        float angle;
        str = pTransformation->GetText();

        eResult = pTransformation->QueryIntAttribute("id", &id);
        sscanf(str, "%f %f %f %f", &angle, &axis.x, &axis.y, &axis.z);

        rotations.push_back(new Rotation(id, axis, angle));
        pTransformation = pTransformation->NextSiblingElement("Rotation");
    }

    if (pElement != nullptr)
        pTransformation = pElement->FirstChildElement("Translation");
    while (pTransformation != nullptr)
    {
        int id;
        float trX;
        float trY;
        float trZ;

        str = pTransformation->GetText();

        eResult = pTransformation->QueryIntAttribute("id", &id);
        sscanf(str, "%f %f %f", &trX, &trY, &trZ);

        translations.push_back(new Translation(id, trX, trY, trZ));
        pTransformation = pTransformation->NextSiblingElement("Translation");
    }

    // Parse vertex data
    pElement = pRoot->FirstChildElement("VertexData");
    int cursor = 0;
    Vector3f tmpPoint;
    if(pElement != nullptr)
    {
        str = pElement->GetText();
        while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
            cursor++;
        while (str[cursor] != '\0')
        {
            for (int cnt = 0; cnt < 3; cnt++)
            {
                if (cnt == 0)
                    tmpPoint.x = atof(str + cursor);
                else if (cnt == 1)
                    tmpPoint.y = atof(str + cursor);
                else
                    tmpPoint.z = atof(str + cursor);
                while (str[cursor] != ' ' && str[cursor] != '\t' && str[cursor] != '\n')
                    cursor++;
                while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
                    cursor++;
            }
            vertices.push_back(tmpPoint);
            vertexCoords.push_back(Vector2f{});
        }
    }

    // Parse texture coordinates
    pElement = pRoot->FirstChildElement("TexCoordData");
    if(pElement != nullptr)
    {
        int countt = 0;
        cursor = 0;
        Vector2f tmp2dPoint;
        str = pElement->GetText();
        while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
            cursor++;
        while (str[cursor] != '\0')
        {
            for (int cnt = 0; cnt < 2; cnt++)
            {
                if (cnt == 0)
                    tmp2dPoint.x = atof(str + cursor);
                else if (cnt == 1)
                    tmp2dPoint.y = atof(str + cursor);
                while (str[cursor] != ' ' && str[cursor] != '\t' && str[cursor] != '\n')
                    cursor++;
                while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
                    cursor++;
            }
            vertexCoords[countt] = tmp2dPoint;
            // vertexCoords.push_back(tmp2dPoint);
            countt++;
        }
    }
    else
    {
        vertexCoords.push_back(Vector2f{});
    }
    

    for(Vector2f& vec : vertexCoords)
    {
        // std::cout << vec;
        ;
    }

    //system("pause");

    // Parse objects
    pElement = pRoot->FirstChildElement("Objects");

    // Parse spheres
    XMLElement *pObject = pElement->FirstChildElement("Sphere");
    XMLElement *objElement;
    while (pObject != nullptr)
    {
        int id;
        int matIndex;
        int cIndex;
        float R;

        glm::mat4 dummy = glm::mat4(1);
        Transform *objTransform = new Transform(dummy);
        std::vector<Texture *> texs;

        objElement = pObject->FirstChildElement("Transformations");
        if (objElement != nullptr)
        {
            str = objElement->GetText();
            const char *ch = str;
            ComputeTransformMatrix(ch, *objTransform);
        }

        eResult = pObject->QueryIntAttribute("id", &id);
        objElement = pObject->FirstChildElement("Material");
        eResult = objElement->QueryIntText(&matIndex);
        objElement = pObject->FirstChildElement("Center");
        eResult = objElement->QueryIntText(&cIndex);
        objElement = pObject->FirstChildElement("Radius");
        eResult = objElement->QueryFloatText(&R);


        objElement = pObject->FirstChildElement("Textures");
        if(objElement != nullptr)
        {
            std::vector<int> texIds;

            const char* tex = objElement->GetText();
            while(*tex != '\0')
            {
                if(*tex == ' ')
                {
                    tex++;
                    continue;
                }
                texIds.push_back(*tex);

                tex++;
            }
            
            for(int i : texIds)
                texs.push_back(this->textures[i - 49]);
        }

        objects.push_back(new Sphere(id, this->materials[matIndex - 1], R, this->vertices[cIndex - 1], objTransform));
        primitives.push_back(new Primitive(objects.back(), objects.back()->mat));

        objects.back()->SetTextures(texs);

        pObject = pObject->NextSiblingElement("Sphere");
    }

    //system("pause");
    // Parse triangles
    pObject = pElement->FirstChildElement("Triangle");
    while (pObject != nullptr)
    {
        int id;
        int matIndex;
        int p1Index;
        int p2Index;
        int p3Index;

        glm::mat4 dummy = glm::mat4(1);
        Transform *objTransform = new Transform(dummy);
        std::vector<Texture *> texs;

        eResult = pObject->QueryIntAttribute("id", &id);
        objElement = pObject->FirstChildElement("Material");
        eResult = objElement->QueryIntText(&matIndex);
        objElement = pObject->FirstChildElement("Indices");
        str = objElement->GetText();
        sscanf(str, "%d %d %d", &p1Index, &p2Index, &p3Index);

        objElement = pObject->FirstChildElement("Transformations");
        if (objElement != nullptr)
        {
            str = objElement->GetText();
            const char *ch = str;
            ComputeTransformMatrix(ch, *objTransform);
        }

        objElement = pObject->FirstChildElement("Textures");
        if (objElement != nullptr)
        {
            std::vector<int> texIds;

            const char *tex = objElement->GetText();
            while (*tex != '\0')
            {
                if (*tex == ' ')
                {
                    tex++;
                    continue;
                }
                texIds.push_back(*tex);

                tex++;
            }

            for (int i : texIds)
                texs.push_back(this->textures[i - 49]);
        }

        objects.push_back(new Triangle(id, this->materials[matIndex - 1], this->vertices[p1Index - 1], this->vertices[p2Index - 1], this->vertices[p3Index - 1],
                                       this->vertexCoords[p1Index - 1], this->vertexCoords[p2Index - 1], this->vertexCoords[p3Index - 1], nullptr));
        primitives.push_back(new Primitive(objects.back(), objects.back()->mat));

        objects.back()->SetTextures(texs);

        pObject = pObject->NextSiblingElement("Triangle");
    }

    // Parse meshes
    pObject = pElement->FirstChildElement("Mesh");
    // pObject = nullptr;
    while (pObject != nullptr)
    {
        enum FileType { DEFAULT, PLY };

        Vector3f motBlur{};
        FileType fType = FileType::DEFAULT;
        Shape::ShadingMode sMode = Shape::ShadingMode::DEFAULT;
        int id;
        int matIndex;
        int p1Index;
        int p2Index;
        int p3Index;
        int cursor = 0;
        int vertexOffset = 0;
        int textureOffset = 0;
        std::vector<std::pair<int, Material*>> faces;
        std::vector<Vector3f*> *meshIndices = new std::vector<Vector3f*>;
        std::vector<Vector2f*> *meshUVs = new std::vector<Vector2f*>;
        std::vector<Texture*> texs;

        glm::mat4 dummy = glm::mat4(1);
        Transform* objTransform = new Transform(dummy);

        const char *attr = pObject->Attribute("shadingMode");
        if (attr != nullptr && strcmp(attr, "smooth") == 0)
            sMode = Shape::ShadingMode::SMOOTH;

        eResult = pObject->QueryIntAttribute("id", &id);
        objElement = pObject->FirstChildElement("Material");
        eResult = objElement->QueryIntText(&matIndex);

        objElement = pObject->FirstChildElement("MotionBlur");
        if (objElement != nullptr)
        {
            str = objElement->GetText();
            sscanf(str, "%f %f %f", &motBlur.x, &motBlur.y, &motBlur.z);
        }


        objElement = pObject->FirstChildElement("Transformations");
        if(objElement != nullptr)
        {
            str = objElement->GetText();
            const char* ch = str;
            ComputeTransformMatrix(ch, *objTransform);
        }

        objElement = pObject->FirstChildElement("Faces");
        objElement->QueryIntAttribute("vertexOffset", &vertexOffset);
        objElement->QueryIntAttribute("textureOffset", &textureOffset);

        
        attr = objElement->Attribute("plyFile");
        if(attr != nullptr)
            fType = FileType::PLY;
        
        switch(fType)
        {
            case FileType::DEFAULT:
            {
                str = objElement->GetText();
                while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
                    cursor++;
                while (str[cursor] != '\0')
                {
                    for (int cnt = 0; cnt < 3; cnt++)
                    {
                        if (cnt == 0)
                            p1Index = atoi(str + cursor);
                        else if (cnt == 1)
                            p2Index = atoi(str + cursor);
                        else
                            p3Index = atoi(str + cursor);
                        while (str[cursor] != ' ' && str[cursor] != '\t' && str[cursor] != '\n')
                            cursor++;
                        while (str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
                            cursor++;
                    }
                    faces.push_back(std::make_pair(-1, this->materials[matIndex - 1]));

                    meshIndices->push_back(&(this->vertices[p1Index - 1 + vertexOffset]));
                    meshIndices->push_back(&(this->vertices[p2Index - 1 + vertexOffset]));
                    meshIndices->push_back(&(this->vertices[p3Index - 1 + vertexOffset]));

                    if(this->vertexCoords.size() == 1)
                    {
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                    }
                    else
                    {
                        meshUVs->push_back(&(this->vertexCoords[p1Index - 1 + textureOffset]));
                        meshUVs->push_back(&(this->vertexCoords[p2Index - 1 + textureOffset]));
                        meshUVs->push_back(&(this->vertexCoords[p3Index - 1 + textureOffset]));
                    }
                }
                
                objects.push_back(new Mesh(id, this->materials[matIndex - 1], faces, meshIndices, meshUVs, objTransform, sMode));
            }
            break;
            case FileType::PLY:
            {
                std::string resLoc("scenes/");


                resLoc += std::string(attr);

                std::cout << "Reading from path: " << resLoc << "\n";
                //system("pause");
                happly::PLYData plyIn(resLoc);
                std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
                std::vector<std::vector<size_t>> fInd = plyIn.getFaceIndices<size_t>();

                std::vector<Vector3f> vertexPos;
                for(const std::array<double, 3>& a : vPos)
                {
                    vertexPos.push_back(Vector3f(a[0], a[1], a[2]));
                }

                for(const std::vector<size_t>& v : fInd)
                {
                    if(v.size() == 4)
                    {
                        faces.push_back(std::make_pair(-1, this->materials[matIndex - 1]));
                        faces.push_back(std::make_pair(-1, this->materials[matIndex - 1]));
                        meshIndices->push_back(&(vertexPos[v[0]]));
                        meshIndices->push_back(&(vertexPos[v[1]]));
                        meshIndices->push_back(&(vertexPos[v[2]]));
                        meshIndices->push_back(&(vertexPos[v[2]]));
                        meshIndices->push_back(&(vertexPos[v[3]]));
                        meshIndices->push_back(&(vertexPos[v[0]]));

                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                        meshUVs->push_back(&(this->vertexCoords[0]));
                    }
                    else if(v.size() == 3)
                    {
                        for (int i = 0; i < 3; ++i)
                        {
                     
                            meshIndices->push_back(&(vertexPos[v[i]]));
                            meshUVs->push_back(&(this->vertexCoords[0]));
                        }
                        faces.push_back(std::make_pair(-1, this->materials[matIndex - 1]));
                    }
                }

                objects.push_back(new Mesh(id, this->materials[matIndex - 1], faces, meshIndices, meshUVs, objTransform, sMode));
                
            }
            break;

            default:
                std::cout << "Invalid file type\n";
        }

        objElement = pObject->FirstChildElement("Textures");
        if (objElement != nullptr)
        {
            std::vector<int> texIds;

            const char *tex = objElement->GetText();
            while (*tex != '\0')
            {
                if (*tex == ' ')
                {
                    tex++;
                    continue;
                }
                texIds.push_back(*tex);

                tex++;
            }

            for (int i : texIds)
                texs.push_back(this->textures[i - 49]);
        }

        objects.back()->SetMotionBlur(motBlur);
        objects.back()->SetTextures(texs);
        meshIndices->erase(meshIndices->begin(), meshIndices->end());
        meshIndices->clear();

        pObject = pObject->NextSiblingElement("Mesh");
    }

    std::cout << "Read all meshes\n";
    // Parse mesh instances
    pObject = pElement->FirstChildElement("MeshInstance");
    // pObject = nullptr;
    while(pObject != nullptr)
    {
        int id;
        int baseMeshID;
        bool resetTransform;

        Vector3f motBlur{};
        
        eResult = pObject->QueryIntAttribute("id", &id);
        eResult = pObject->QueryIntAttribute("baseMeshId", &baseMeshID);
        eResult = pObject->QueryBoolAttribute("resetTransform", &resetTransform);

        

        objElement = pObject->FirstChildElement("MotionBlur");
        if(objElement != nullptr)
        {
            str = objElement->GetText();
            sscanf(str, "%f %f %f", &motBlur.x, &motBlur.y, &motBlur.z);
        }


        Shape* soughtMesh = GetMeshWithID(baseMeshID, Shape::ShapeType::MESH);

        Shape* newMesh = soughtMesh->Clone(resetTransform);

        int matIndex;

        objElement = pObject->FirstChildElement("Material");
        if(objElement != nullptr)
        {
            
            eResult = objElement->QueryIntText(&matIndex);

            newMesh->SetMaterial(materials[matIndex - 1]);
        }
        
        glm::mat4 dummy = glm::mat4(1);
        Transform *objTransform = new Transform(dummy);

        objElement = pObject->FirstChildElement("Transformations");
        if (objElement != nullptr)
        {
            str = objElement->GetText();

            const char *ch = str;
            ComputeTransformMatrix(ch, *objTransform);
        }

        newMesh->SetTransformation(objTransform);
        newMesh->SetMotionBlur(motBlur);

        objects.push_back(newMesh);

        pObject = pObject->NextSiblingElement("MeshInstance");
    }


    // Parse lights
    int id;
    
    pElement = pRoot->FirstChildElement("Lights");

    XMLElement *pLight = pElement->FirstChildElement("AmbientLight");
    XMLElement *lightElement;

    if(pLight != nullptr)
    {
        str = pLight->GetText();
        sscanf(str, "%f %f %f", &ambientLight.r, &ambientLight.g, &ambientLight.b);
    }

    std::cout << "Read ambient\n";
    //system("pause");

    pLight = pElement->FirstChildElement("PointLight");
    while (pLight != nullptr)
    {
        Vector3f position;
        Vector3f intensity;

        eResult = pLight->QueryIntAttribute("id", &id);
        lightElement = pLight->FirstChildElement("Position");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &position.x, &position.y, &position.z);
        lightElement = pLight->FirstChildElement("Intensity");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &intensity.r, &intensity.g, &intensity.b);

        lights.push_back(new PointLight(position, intensity));

        pLight = pLight->NextSiblingElement("PointLight");
    }

    std::cout << "Read point\n";

    pLight = pElement->FirstChildElement("DirectionalLight");
    while(pLight != nullptr)
    {
        Vector3f direction;
        Vector3f radiance;

        eResult = pLight->QueryIntAttribute("id", &id);
        lightElement = pLight->FirstChildElement("Direction");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &direction.x, &direction.y, &direction.z);
        lightElement = pLight->FirstChildElement("Radiance");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &radiance.r, &radiance.g, &radiance.b);

        lights.push_back(new DirectionalLight(Vector3f{}, radiance, direction));

        pLight = pLight->NextSiblingElement("DirectionalLight");
    }
    std::cout << "Read directional\n";

    pLight = pElement->FirstChildElement("SpotLight");
    while (pLight != nullptr)
    {
        Vector3f position;
        Vector3f direction;
        Vector3f intensity;
        float coverageAngle;
        float falloffAngle;
        float exponent;

        eResult = pLight->QueryIntAttribute("id", &id);
        lightElement = pLight->FirstChildElement("Direction");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &direction.x, &direction.y, &direction.z);
        lightElement = pLight->FirstChildElement("Intensity");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &intensity.r, &intensity.g, &intensity.b);
        lightElement = pLight->FirstChildElement("Position");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &position.r, &position.g, &position.b);
        lightElement = pLight->FirstChildElement("CoverageAngle");
        lightElement->QueryFloatText(&coverageAngle);
        lightElement = pLight->FirstChildElement("FalloffAngle");
        lightElement->QueryFloatText(&falloffAngle);

        lights.push_back(new SpotLight(position, intensity, direction, coverageAngle, falloffAngle, exponent));

        pLight = pLight->NextSiblingElement("SpotLight");
    }
    std::cout << "Read spot\n";

    pLight = pElement->FirstChildElement("AreaLight");
    while (pLight != nullptr)
    {
        Vector3f position;
        Vector3f normal;
        Vector3f radiance;
        float size;

        eResult = pLight->QueryIntAttribute("id", &id);
        lightElement = pLight->FirstChildElement("Position");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &position.x, &position.y, &position.z);
        lightElement = pLight->FirstChildElement("Normal");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &normal.r, &normal.g, &normal.b);
        lightElement = pLight->FirstChildElement("Radiance");
        str = lightElement->GetText();
        sscanf(str, "%f %f %f", &radiance.r, &radiance.g, &radiance.b);
        lightElement = pLight->FirstChildElement("Size");
        lightElement->QueryFloatText(&size);

        lights.push_back(new AreaLight(position, radiance, normal, size));

        pLight = pLight->NextSiblingElement("AreaLight");

    }

    pLight = pElement->FirstChildElement("SphericalDirectionalLight");
    while (pLight != nullptr)
    {
        int imID;
    

        eResult = pLight->QueryIntAttribute("id", &id);
        lightElement = pLight->FirstChildElement("ImageId");
        lightElement->QueryIntText(&imID);

        Texture* tex;

        if (imagePaths[imID - 1].back() == 'r')
        {
            TMOData dat = Tonemapper::ReadExr(imagePaths[imID - 1]);
            tex = new ImageTexture(imagePaths[imID - 1], 2, Texture::DecalMode::BLEND_KD, 1, ImageTexture::InterpolationType::NEAREST, 1, dat.data);
            tex->width = dat.width;
            tex->height = dat.height;
        }
        else
        {
            tex = new ImageTexture(imagePaths[imID - 1], 2, Texture::DecalMode::BLEND_KD, 1, ImageTexture::InterpolationType::NEAREST, 1);
        }
        

        lights.push_back(new EnvironmentLight(tex));

        pLight = pLight->NextSiblingElement("SphericalDirectionalLight");
    }
    std::cout << "Sphreical read\n";

}

}
