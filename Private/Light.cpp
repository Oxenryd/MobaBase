#include "Light.hpp"

Light::operator LightComponent& () {
	return m_reg->get<GPULight>(m_entity);
}

