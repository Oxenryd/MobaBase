#ifndef TRANSFORM_COMPONENT_HPP
#define TRANSFORM_COMPONENT_HPP

#include "ObjectState.hpp"

//#include <glm/glm.hpp>
//#include <glm/gtx/quaternion.hpp>
//#include "MMath.hpp"

struct TransformComponent
{
	uint32_t dataIndex{ UINT32_INVALID };
	//glm::vec3 position{ 0, 0, 0 };
	//glm::quat rotation{ 1, 0, 0, 0 };
	//glm::vec3 scale{ 1,1,1 };
	uint16_t sceneIndex{ UINT16_INVALID };
	StateField state{ 0 };
	void* callbackUserData = nullptr;
	void (*onDirtyCallback)(void* userData) = nullptr;

	//INLINE glm::mat4x4 trs() const {
	//	return MMath::composeTRS(position, rotation, scale);
	//}
	//
	//INLINE glm::mat4x4 trs_inv() {
	//	return MMath::composeTRS_Inverse(position, rotation, scale);
	//}


};

#endif