#pragma once

namespace actracer
{

class Scene;
class Transform;

class SceneParser
{
public:
	static Scene* CreateSceneFromXML(const char* filePath);
private:
	static Transform& ComputeTransformMatrix(Scene* scene, const char *ch, Transform &objTransform);
private:
	SceneParser() = default;
};

}