//#ifndef MATERIAL_INSTANCE_H
//#define MATERIAL_INSTANCE_H
//
//#include <cstdint>
//#include <string>
//#include "Material.hpp"
//
//class MaterialInstance
//{
//private:
//	uint32_t m_instanceIndex;
//
//public:
//	MaterialInstance(Material* const base, size_t index) :
//		base{ base },
//		m_instanceIndex{ static_cast<uint32_t>(index) } {}
//	Material* const base;
//	void* const pushDataPtr() { return nullptr; }
//	template <typename T>
//	T* readParameter(const std::string& paramName, uint32_t instanceIndex = 0) {
//		return base->
//	}
//
//};
//
//#endif