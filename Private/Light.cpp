#include "Light.hpp"

Light::operator const LightComponent& () const {
	return m_reg->get<LightComponent>(m_entity);
}

