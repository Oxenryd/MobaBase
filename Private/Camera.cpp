#include "Camera.hpp"
#include "Engine.h"


Camera* Camera::getCamera(uint16_t sceneIndex, uint32_t camIndex) {

	auto scene = Engine::getInstance()->getScene(sceneIndex);
	if (!scene)
		return nullptr;

	auto& cameras = scene->gameObjectSystem().getAllOfType<Camera>();

	if (camIndex >= cameras.size() || cameras.empty())
		return nullptr;

	return &cameras[camIndex];
}

Camera* Camera::getCamera(const CamIndex& camIndex) {
	return getCamera(camIndex.sceneIndex, camIndex.camIndex);
}
