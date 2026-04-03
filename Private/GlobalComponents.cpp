#include "GlobalComponents.hpp"
#include "GlobalSystem.hpp"
#include "Engine.h"

Layer::operator LayerMask& () {
	return Engine::getInstance()->getGlobalSystem().registry().get<LayerComponent>(m_entity);
}

Layer::operator const LayerMask& () const {
	return Engine::getInstance()->getGlobalSystem().registry().get<LayerComponent>(m_entity);
}

