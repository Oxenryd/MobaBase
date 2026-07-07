#include "SystemECS.h"
#include "Engine.h"

SceneBase* SystemECS::getScene() {
	return Engine::getInstance()->getScene(m_sceneIndex);
}
