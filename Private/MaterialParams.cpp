#include "MaterialParams.h"
#include "ShaderManager.h"

std::string& MatParam::name() {
	return ShaderManager::getInstance()->paramNames()[nameIndex];
}
std::string& MatParam::name() const {
	return ShaderManager::getInstance()->paramNames()[nameIndex];
}

std::string& MatParam::parentName() {
	return ShaderManager::getInstance()->paramNames()[parentNameIndex];
}

std::string& MatParam::parentName() const {
	return ShaderManager::getInstance()->paramNames()[parentNameIndex];
}

MaterialBuffer::MaterialBuffer(const MatParam& structure) :
	m_size{ 0 },
	m_capacity{ 0 },
	m_entrySize{ structure.paddedVarSize() },
	m_memory{ nullptr },
	m_bind{ structure.bindingIndex, structure.setIndex },
	m_bufferType{ Uniform } {
	assert(structure.members.size() > 0
		   && "MaterialBuffer: must pass MatParam& structure with members.size() > 0");

	m_bindless = structure.arrayType == MatParamArrayType::Dynamic ? 1 : 0;
	m_stage = structure.stage;
	m_nameIndex = structure.nameIndex;

	for (auto& member : structure.members) {
		const auto name = ShaderManager::getInstance()->getParamName(member.nameIndex);
		m_indexNameMap.insert({ m_structure.size(), name });
		m_nameIndexMap.insert({ name, m_structure.size() });
		m_structure.push_back(member);
	}
	switch (structure.type) {
		case TypeBase::CBuffer:
			m_bufferType = Uniform; break;
		case TypeBase::PushConstStruct:
			m_bufferType = PushConstant; break;
		case TypeBase::Struct:
			m_bufferType = SSBO; break;
	}
}