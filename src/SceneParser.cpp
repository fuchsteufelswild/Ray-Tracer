#include "tinyxml2.h"
#include "happly.h"

#include "SceneParser.h"
#include "Scene.h"
#include "Camera.h"
#include "Tonemapper.h"
#include "brdf.h"
#include "Material.h"
#include "Texture.h"
#include "PerlinTextureImpl.h"
#include "ImageTextureImpl.h"

#include "TextureReader.h"

#include "NormalChangerTexture.h"
#include "ColorChangerTexture.h"

#include "TextureValueRetrieveMethod.h"

#include "Sphere.h"
#include "Triangle.h"
#include "Mesh.h"
#include "Primitive.h"

#include "Light.h"


using namespace tinyxml2; 

namespace actracer
{

/*
 * TODO: Refactor method
 * Decompose into multiple functions that are responsible for
 * parsing specific types of elements
 */ 
Scene* SceneParser::CreateSceneFromXML(const char* filePath)
{
	Scene* scene = new Scene();

	const char *str;
	XMLDocument xmlDoc;
	XMLError eResult;
	XMLElement *pElement;

	eResult = xmlDoc.LoadFile(filePath);

	if(eResult != XMLError::XML_SUCCESS)
	{
		std::cout << "Error reading input scene file\n";
		return nullptr; 
	}

	XMLNode *pRoot = xmlDoc.FirstChild();

	// Recursion depth
	pElement = pRoot->FirstChildElement("MaxRecursionDepth");
	if (pElement != nullptr)
		pElement->QueryIntText(&scene->maxRecursionDepth);

	// Background color
	pElement = pRoot->FirstChildElement("BackgroundColor");
	if (pElement != nullptr)
	{
		str = pElement->GetText();
		sscanf(str, "%f %f %f", &scene->backgroundColor.r, &scene->backgroundColor.g, &scene->backgroundColor.b);
	}
	// Shadow epsilon
	pElement = pRoot->FirstChildElement("ShadowRayEpsilon");
	if (pElement != nullptr)
		pElement->QueryFloatText(&scene->shadowRayEps);

	// Intersection epsilon
	pElement = pRoot->FirstChildElement("intersectionTestEpsilon");
	if (pElement != nullptr)
		eResult = pElement->QueryFloatText(&scene->intTestEps);

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
		Camera::ImagePlane imgPlane;
		float focalDistance = 0.0f;
		float apertureSize = 0.0f;
		int mode = 0;  // Default mode is projective
		int iType = 0; // Default input type is ( l, r, b, t, d )

		eResult = pCamera->QueryIntAttribute("id", &id); // Cam id

		const char *attr = pCamera->Attribute("type");

		if (attr != nullptr)
			if (strcmp(attr, "lookAt") == 0)
				iType = 1;

		// Parse the common attributes ( they will exist in every xml file )
		camElement = pCamera->FirstChildElement("Position");
		str = camElement->GetText();
		sscanf(str, "%f %f %f", &pos.x, &pos.y, &pos.z);

		camElement = pCamera->FirstChildElement("Up");
		str = camElement->GetText();
		sscanf(str, "%f %f %f", &up.x, &up.y, &up.z);

		camElement = pCamera->FirstChildElement("NumSamples");
		if (camElement != nullptr)
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

		if (!iType) // Use directly given boundaries
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
			float fovY = -1;	// Field of view of the frustum planes on sides
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
		if (camElement != nullptr)
		{
			std::string type;
			float key, burn;
			float saturation;
			float gamma;

			XMLElement *toneElement;
			toneElement = camElement->FirstChildElement("TMOOptions");
			str = toneElement->GetText();
			sscanf(str, "%f %f", &key, &burn);

			toneElement = camElement->FirstChildElement("Saturation");
			toneElement->QueryFloatText(&saturation);
			toneElement = camElement->FirstChildElement("Gamma");
			toneElement->QueryFloatText(&gamma);

			scene->tmo = new Tonemapper(key, burn, saturation, gamma);
		}

		scene->cameras.push_back(new Camera(id, imageName, pos, gaze, up, imgPlane, numSamples, PixelSampleMethod::JITTERED, focalDistance, apertureSize));

		pCamera = pCamera->NextSiblingElement("Camera");
	}

	// Parse BRDFs
	pElement = pRoot->FirstChildElement("BRDFs");

	if (pElement != nullptr)
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

			scene->brdfs.push_back(new BRDFBlinnPhongModified(_phong, _normalized));

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

			scene->brdfs.push_back(new BRDFPhongOriginal(_phong));

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

			scene->brdfs.push_back(new BRDFBlinnPhongOriginal(_phong));

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

			scene->brdfs.push_back(new BRDFPhongModified(_phong, _normalized));

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

			scene->brdfs.push_back(new BRDFTorranceSparrow(_phong, _kdfresnel));

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
		int curr = scene->materials.size() - 1;

		Vector3f _ARC;
		Vector3f _DRC;
		Vector3f _SRC;
		Vector3f _MRC;
		Vector3f _AC;
		float _AI;
		float _RI;
		float _roughness;
		int _phong = 1;
		int _id;
		int _brdfID = -1;
		BRDFBase *matBRDF = nullptr;

		Material::MatType _typ = Material::MatType::DEFAULT;

		eResult = pMaterial->QueryIntAttribute("id", &_id);
		eResult = pMaterial->QueryBoolAttribute("degamma", &degamma);
		eResult = pMaterial->QueryIntAttribute("BRDF", &_brdfID);
		const char *attr = pMaterial->Attribute("type");

		if (attr != nullptr)
		{
			if (strcmp(attr, "mirror") == 0)
				_typ = Material::MatType::MIRROR;
			else if (strcmp(attr, "dielectric") == 0)
				_typ = Material::MatType::DIELECTRIC;
			else if (strcmp(attr, "conductor") == 0)
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
		if (materialElement != nullptr)
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
		if (materialElement != nullptr)
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

		if (_brdfID != -1)
			matBRDF = scene->brdfs[_brdfID - 1];
		else
		{
			matBRDF = new BRDFBlinnPhongOriginal(_phong);
		}

		if (degamma)
		{
			_DRC.x = std::pow(_DRC.x, 2.2f);
			_DRC.y = std::pow(_DRC.y, 2.2f);
			_DRC.z = std::pow(_DRC.z, 2.2f);

			_SRC.x = std::pow(_SRC.x, 2.2f);
			_SRC.y = std::pow(_SRC.y, 2.2f);
			_SRC.z = std::pow(_SRC.z, 2.2f);

			_ARC.x = std::pow(_ARC.x, 2.2f);
			_ARC.y = std::pow(_ARC.y, 2.2f);
			_ARC.z = std::pow(_ARC.z, 2.2f);
		}

		switch (_typ)
		{
		case Material::MatType::CONDUCTOR:
			scene->materials.push_back(new Conductor(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness, _AI));
			break;
		case Material::MatType::DIELECTRIC:
			scene->materials.push_back(new Dielectric(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness, _AC));
			break;
		default:
			scene->materials.push_back(new Material(_id, _typ, _DRC, _SRC, _ARC, _MRC, _RI, 1, _phong, _roughness));
			break;
		}

		scene->materials.back()->SetDegamma(degamma);
		scene->materials.back()->SetBRDF(matBRDF);

		if (matBRDF)
			scene->materials.back()->GetBRDF()->ai = _AI;

		// if (scene->materials.back()->degamma)
		// {
		// 	scene->materials.back()->DRC.x = std::pow(scene->materials.back()->DRC.x, 2.2f);
		// 	scene->materials.back()->DRC.y = std::pow(scene->materials.back()->DRC.y, 2.2f);
		// 	scene->materials.back()->DRC.z = std::pow(scene->materials.back()->DRC.z, 2.2f);

		// 	scene->materials.back()->SRC.x = std::pow(scene->materials.back()->SRC.x, 2.2f);
		// 	scene->materials.back()->SRC.y = std::pow(scene->materials.back()->SRC.y, 2.2f);
		// 	scene->materials.back()->SRC.z = std::pow(scene->materials.back()->SRC.z, 2.2f);

		// 	scene->materials.back()->ARC.x = std::pow(scene->materials.back()->ARC.x, 2.2f);
		// 	scene->materials.back()->ARC.y = std::pow(scene->materials.back()->ARC.y, 2.2f);
		// 	scene->materials.back()->ARC.z = std::pow(scene->materials.back()->ARC.z, 2.2f);
		// }

		

		pMaterial = pMaterial->NextSiblingElement("Material");
	}

	pElement = pRoot->FirstChildElement("Textures");

	if (pElement != nullptr)
	{
		XMLElement *textureElement = pElement->FirstChildElement("Images");
		if (textureElement != nullptr)
		{
			XMLElement *imageElement = textureElement->FirstChildElement("Image");

			while (imageElement != nullptr)
			{
				int id;
				const char *imPath;

				eResult = imageElement->QueryIntAttribute("id", &id);

				imPath = imageElement->GetText();
				if (imPath != nullptr)
					scene->imagePaths.push_back(std::string(imPath));

				imageElement = imageElement->NextSiblingElement("Image");
			}
		}
		textureElement = pElement->FirstChildElement("TextureMap");

		while (textureElement != nullptr)
		{
			XMLElement *miniElement;

			int id;

			NoiseConversionType nct = NoiseConversionType::LINEAR;
			InterpolationMethodCode itype = InterpolationMethodCode::NEAREST;
			DecalMode decalMode;
			TextureType ttype;
			ImageType imType;
			float noiseScale = 1.0f;
			float bumpFactor = 1.0f;
			int normalizer = 255;
			int imageID;

			Texture* createdTexture;

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
					decalMode = DecalMode::BUMP_NORMAL;
					createdTexture = new NormalBumper();
				}
				else if (strcmp(decal, "replace_kd") == 0)
				{
					decalMode = DecalMode::REPLACE_KD;
					createdTexture = new KDReplacer();
				}
				else if (strcmp(decal, "replace_normal") == 0)
				{
					decalMode = DecalMode::REPLACE_NORMAL;
					createdTexture = new NormalReplacer();
				}
				else if (strcmp(decal, "replace_background") == 0)
				{
					decalMode = DecalMode::REPLACE_BACKGROUND;
					createdTexture = new Texture();
				}
				else if (strcmp(decal, "replace_all") == 0)
				{
					decalMode = DecalMode::REPLACE_ALL;
					createdTexture = new ColorReplacer();
				}
				else if (strcmp(decal, "blend_kd") == 0)
				{
					decalMode = DecalMode::BLEND_KD;
					createdTexture = new KDBlender();
				}
			}



			const char *attrType = textureElement->Attribute("type");

			if (attrType != nullptr)
			{
				if (strcmp(attrType, "perlin") == 0)
				{
					ttype = TextureType::PERLIN;
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
								nct = NoiseConversionType::ABSVAL;
							else
								nct = NoiseConversionType::LINEAR;
						}
					}

					createdTexture->SetupPerlinTexture(bumpFactor, noiseScale, nct);
					// scene->textures.push_back(new PerlinTexture(id, decalMode, bumpFactor, noiseScale, nct));
				}
				else if (strcmp(attrType, "image") == 0)
				{
					ttype = TextureType::IMAGE;

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
								itype == InterpolationMethodCode::BILINEAR;
							else if (strcmp(attrType, "nearest") == 0)
								itype == InterpolationMethodCode::NEAREST;
						}
					}

					if (scene->imagePaths[imageID - 1].back() == 'r')
						imType = ImageType::EXR;
					else
						imType = ImageType::PNG;

					createdTexture->SetupImageTexture(scene->imagePaths[imageID - 1], bumpFactor, normalizer, imType, itype);
				}

				scene->textures.push_back(createdTexture);
			}

			if (decalMode == DecalMode::REPLACE_BACKGROUND)
				scene->bgTexture = scene->textures.back();

			textureElement = textureElement->NextSiblingElement("TextureMap");
		}
	}

	pElement = pRoot->FirstChildElement("Transformations");
	XMLElement *pTransformation;
	if (pElement != nullptr)
		pTransformation = pElement->FirstChildElement("Scaling");
	while (pTransformation != nullptr)
	{
		int id;
		float scaleX;
		float scaleY;
		float scaleZ;

		str = pTransformation->GetText();

		eResult = pTransformation->QueryIntAttribute("id", &id);
		sscanf(str, "%f %f %f", &scaleX, &scaleY, &scaleZ);

		// transformations.push_back(new Scaling(id, scaleX, scaleY, scaleZ));
		scene->scalings.push_back(new Scaling(id, scaleX, scaleY, scaleZ));
		pTransformation = pTransformation->NextSiblingElement("Scaling");
	}

	if (pElement != nullptr)
		pTransformation = pElement->FirstChildElement("Rotation");
	while (pTransformation != nullptr)
	{
		int id;
		glm::vec3 axis;
		float angle;
		str = pTransformation->GetText();

		eResult = pTransformation->QueryIntAttribute("id", &id);
		sscanf(str, "%f %f %f %f", &angle, &axis.x, &axis.y, &axis.z);

		scene->rotations.push_back(new Rotation(id, axis, angle));
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

		scene->translations.push_back(new Translation(id, trX, trY, trZ));
		pTransformation = pTransformation->NextSiblingElement("Translation");
	}

	// Parse vertex data
	pElement = pRoot->FirstChildElement("VertexData");
	int cursor = 0;
	Vector3f tmpPoint;
	if (pElement != nullptr)
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
			scene->vertices.push_back(tmpPoint);
			scene->vertexCoords.push_back(Vector2f{});
		}
	}

	// Parse texture coordinates
	pElement = pRoot->FirstChildElement("TexCoordData");
	if (pElement != nullptr)
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
			scene->vertexCoords[countt] = tmp2dPoint;
			// vertexCoords.push_back(tmp2dPoint);
			countt++;
		}
	}
	else
	{
		scene->vertexCoords.push_back(Vector2f{});
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

		ColorChangerTexture* colorChanger = nullptr;
		NormalChangerTexture* normalChanger = nullptr;

		texs.push_back(nullptr);
		texs.push_back(nullptr);

		objElement = pObject->FirstChildElement("Transformations");
		if (objElement != nullptr)
		{
			str = objElement->GetText();
			const char *ch = str;
			ComputeTransformMatrix(scene, ch, *objTransform);
		}

		eResult = pObject->QueryIntAttribute("id", &id);
		objElement = pObject->FirstChildElement("Material");
		eResult = objElement->QueryIntText(&matIndex);
		objElement = pObject->FirstChildElement("Center");
		eResult = objElement->QueryIntText(&cIndex);
		objElement = pObject->FirstChildElement("Radius");
		eResult = objElement->QueryFloatText(&R);

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
			{
				Texture* tex = scene->textures[i - 49];

				NormalChangerTexture* normalPart = dynamic_cast<NormalChangerTexture*>(tex);
				ColorChangerTexture* colorPart = dynamic_cast<ColorChangerTexture*>(tex);

				if (normalPart)
					normalChanger = normalPart;
				else if (colorPart)
					colorChanger = colorPart;
			}
		}

		scene->objects.push_back(new Sphere(id, scene->materials[matIndex - 1], R, scene->vertices[cIndex - 1], objTransform));
		scene->primitives.push_back(new Primitive(scene->objects.back(), scene->objects.back()->mat));

		scene->objects.back()->SetTextures(colorChanger, normalChanger);

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
		ColorChangerTexture *colorChanger = nullptr;
		NormalChangerTexture *normalChanger = nullptr;

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
			ComputeTransformMatrix(scene, ch, *objTransform);
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
			{
				Texture *tex = scene->textures[i - 49];

				NormalChangerTexture *normalPart = dynamic_cast<NormalChangerTexture *>(tex);
				ColorChangerTexture *colorPart = dynamic_cast<ColorChangerTexture *>(tex);

				if (normalPart)
					normalChanger = normalPart;
				else if (colorPart)
					colorChanger = colorPart;
			}
		}

		scene->objects.push_back(new Triangle(id, scene->materials[matIndex - 1], scene->vertices[p1Index - 1], scene->vertices[p2Index - 1], scene->vertices[p3Index - 1],
											  scene->vertexCoords[p1Index - 1], scene->vertexCoords[p2Index - 1], scene->vertexCoords[p3Index - 1], nullptr));
		scene->primitives.push_back(new Primitive(scene->objects.back(), scene->objects.back()->mat));

		scene->objects.back()->SetTextures(colorChanger, normalChanger);

		pObject = pObject->NextSiblingElement("Triangle");
	}

	// Parse meshes
	pObject = pElement->FirstChildElement("Mesh");
	// pObject = nullptr;
	while (pObject != nullptr)
	{
		enum FileType
		{
			DEFAULT,
			PLY
		};

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
		std::vector<std::pair<int, Material *>> faces;
		std::vector<Vector3f *> *meshIndices = new std::vector<Vector3f *>;
		std::vector<Vector2f *> *meshUVs = new std::vector<Vector2f *>;
		std::vector<Texture *> texs;
		ColorChangerTexture *colorChanger = nullptr;
		NormalChangerTexture *normalChanger = nullptr;
		glm::mat4 dummy = glm::mat4(1);
		Transform *objTransform = new Transform(dummy);

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
		if (objElement != nullptr)
		{
			str = objElement->GetText();
			const char *ch = str;
			ComputeTransformMatrix(scene, ch, *objTransform);
		}

		objElement = pObject->FirstChildElement("Faces");
		objElement->QueryIntAttribute("vertexOffset", &vertexOffset);
		objElement->QueryIntAttribute("textureOffset", &textureOffset);

		attr = objElement->Attribute("plyFile");
		if (attr != nullptr)
			fType = FileType::PLY;

		switch (fType)
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
				faces.push_back(std::make_pair(-1, scene->materials[matIndex - 1]));

				meshIndices->push_back(&(scene->vertices[p1Index - 1 + vertexOffset]));
				meshIndices->push_back(&(scene->vertices[p2Index - 1 + vertexOffset]));
				meshIndices->push_back(&(scene->vertices[p3Index - 1 + vertexOffset]));

				if (scene->vertexCoords.size() == 1)
				{
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
				}
				else
				{
					meshUVs->push_back(&(scene->vertexCoords[p1Index - 1 + textureOffset]));
					meshUVs->push_back(&(scene->vertexCoords[p2Index - 1 + textureOffset]));
					meshUVs->push_back(&(scene->vertexCoords[p3Index - 1 + textureOffset]));
				}
			}

			scene->objects.push_back(new Mesh(id, scene->materials[matIndex - 1], faces, meshIndices, meshUVs, objTransform, sMode));
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
			for (const std::array<double, 3> &a : vPos)
			{
				vertexPos.push_back(Vector3f(a[0], a[1], a[2]));
			}

			for (const std::vector<size_t> &v : fInd)
			{
				if (v.size() == 4)
				{
					faces.push_back(std::make_pair(-1, scene->materials[matIndex - 1]));
					faces.push_back(std::make_pair(-1, scene->materials[matIndex - 1]));
					meshIndices->push_back(&(vertexPos[v[0]]));
					meshIndices->push_back(&(vertexPos[v[1]]));
					meshIndices->push_back(&(vertexPos[v[2]]));
					meshIndices->push_back(&(vertexPos[v[2]]));
					meshIndices->push_back(&(vertexPos[v[3]]));
					meshIndices->push_back(&(vertexPos[v[0]]));

					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
					meshUVs->push_back(&(scene->vertexCoords[0]));
				}
				else if (v.size() == 3)
				{
					for (int i = 0; i < 3; ++i)
					{

						meshIndices->push_back(&(vertexPos[v[i]]));
						meshUVs->push_back(&(scene->vertexCoords[0]));
					}
					faces.push_back(std::make_pair(-1, scene->materials[matIndex - 1]));
				}
			}

			scene->objects.push_back(new Mesh(id, scene->materials[matIndex - 1], faces, meshIndices, meshUVs, objTransform, sMode));
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
			{
				Texture *tex = scene->textures[i - 49];

				NormalChangerTexture *normalPart = dynamic_cast<NormalChangerTexture *>(tex);
				ColorChangerTexture *colorPart = dynamic_cast<ColorChangerTexture *>(tex);

				if (normalPart)
					normalChanger = normalPart;
				else if (colorPart)
					colorChanger = colorPart;
			}
		}

		scene->objects.back()->SetMotionBlur(motBlur);
		scene->objects.back()->SetTextures(colorChanger, normalChanger);
		meshIndices->erase(meshIndices->begin(), meshIndices->end());
		meshIndices->clear();

		pObject = pObject->NextSiblingElement("Mesh");
	}

	std::cout << "Read all meshes\n";
	// Parse mesh instances
	pObject = pElement->FirstChildElement("MeshInstance");
	// pObject = nullptr;
	while (pObject != nullptr)
	{
		int id;
		int baseMeshID;
		bool resetTransform;

		Vector3f motBlur{};

		eResult = pObject->QueryIntAttribute("id", &id);
		eResult = pObject->QueryIntAttribute("baseMeshId", &baseMeshID);
		eResult = pObject->QueryBoolAttribute("resetTransform", &resetTransform);

		objElement = pObject->FirstChildElement("MotionBlur");
		if (objElement != nullptr)
		{
			str = objElement->GetText();
			sscanf(str, "%f %f %f", &motBlur.x, &motBlur.y, &motBlur.z);
		}

		Shape *soughtMesh = scene->GetMeshWithID(baseMeshID, Shape::ShapeType::MESH);

		Shape *newMesh = soughtMesh->Clone(resetTransform);

		int matIndex;

		objElement = pObject->FirstChildElement("Material");
		if (objElement != nullptr)
		{

			eResult = objElement->QueryIntText(&matIndex);

			newMesh->SetMaterial(scene->materials[matIndex - 1]);
		}

		glm::mat4 dummy = glm::mat4(1);
		Transform *objTransform = new Transform(dummy);

		objElement = pObject->FirstChildElement("Transformations");
		if (objElement != nullptr)
		{
			str = objElement->GetText();

			const char *ch = str;
			ComputeTransformMatrix(scene, ch, *objTransform);
		}

		newMesh->SetTransformation(objTransform);
		newMesh->SetMotionBlur(motBlur);

		scene->objects.push_back(newMesh);

		pObject = pObject->NextSiblingElement("MeshInstance");
	}

	// Parse lights
	int id;

	pElement = pRoot->FirstChildElement("Lights");

	XMLElement *pLight = pElement->FirstChildElement("AmbientLight");
	XMLElement *lightElement;

	if (pLight != nullptr)
	{
		str = pLight->GetText();
		sscanf(str, "%f %f %f", &scene->ambientLight.r, &scene->ambientLight.g, &scene->ambientLight.b);
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

		scene->lights.push_back(new PointLight(position, intensity));

		pLight = pLight->NextSiblingElement("PointLight");
	}

	std::cout << "Read point\n";

	pLight = pElement->FirstChildElement("DirectionalLight");
	while (pLight != nullptr)
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

		scene->lights.push_back(new DirectionalLight(Vector3f{}, radiance, direction));

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

		scene->lights.push_back(new SpotLight(position, intensity, direction, coverageAngle, falloffAngle, exponent));

		pLight = pLight->NextSiblingElement("SpotLight");
	}
	

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

		scene->lights.push_back(new AreaLight(position, radiance, normal, size));

		pLight = pLight->NextSiblingElement("AreaLight");
	}

	return scene;
}

Transform& SceneParser::ComputeTransformMatrix(Scene* scene, const char *ch, Transform &objTransform)
{
	while (*ch != '\0')
	{
		switch (*ch)
		{
		case 'r':
		{
			int tid = *(ch + 1) - 48;

			for (auto itr = scene->rotations.begin(); itr != scene->rotations.end(); ++itr)
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

			for (auto itr = scene->translations.begin(); itr != scene->translations.end(); ++itr)
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

			for (auto itr = scene->scalings.begin(); itr != scene->scalings.end(); ++itr)
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
}