#ifndef MOBAMATH_HPP
#define MOBAMATH_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MMath
{
	static inline glm::mat4x4 composeTRS(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {

        glm::mat4 rot = glm::mat4_cast(rotation);
        rot[0] *= scale.x;
        rot[1] *= scale.y;
        rot[2] *= scale.z;

        rot[3] = glm::vec4(position, 1.0f);
        return rot;
	}
}

#endif