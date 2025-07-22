#ifndef GAMEOBJECT_SYSTEM_HPP
#define GAMEOBJECT_SYSTEM_HPP

#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>
#include <typeindex>

#include "SystemECS.h"
#include "GameObject.hpp"

struct IGameObjectRange
{
	virtual ~IGameObjectRange() = default;
	virtual size_t size() const = 0;
	virtual GameObject* at(size_t i) = 0;
};

template<typename T>
struct GameObjectRangeImpl : IGameObjectRange
{
	std::vector<T>* vec;

	GameObjectRangeImpl(std::vector<T>* v) : vec(v) {}

	size_t size() const override { return vec->size(); }
	GameObject* at(size_t i) override { return &(*vec)[i]; }
};


class GameObjectSystemIterator
{
public:
	using VectorList = std::vector<std::unique_ptr<IGameObjectRange>>;

	GameObjectSystemIterator(VectorList* sources, size_t outer, size_t inner)
		: m_sources(sources), m_outerIndex(outer), m_innerIndex(inner) {
		advanceToValid();
	}

	GameObjectSystemIterator& operator++() {
		++m_innerIndex;
		advanceToValid();
		return *this;
	}

	GameObject* operator*() const {
		return m_sources->at(m_outerIndex)->at(m_innerIndex);
	}

	bool operator!=(const GameObjectSystemIterator& other) const {
		return m_outerIndex != other.m_outerIndex || m_innerIndex != other.m_innerIndex;
	}

private:
	void advanceToValid() {
		while (m_outerIndex < m_sources->size()) {
			if (m_innerIndex < m_sources->at(m_outerIndex)->size()) {
				return; // valid
			}
			++m_outerIndex;
			m_innerIndex = 0;
		}
	}

	VectorList* m_sources = nullptr;
	size_t m_outerIndex = 0;
	size_t m_innerIndex = 0;
};




class GameObjectSystem : public SystemECS
{
private:
	uint32_t m_sceneIndex = UINT32_INVALID;
	std::unordered_map<std::type_index, void*> m_typeVectors;
	std::vector<std::unique_ptr<IGameObjectRange>> m_ranges;

	template<typename T>
	std::vector<T>& _getVectorForType() {
		static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
		const std::type_index typeId = typeid(T);

		auto it = m_typeVectors.find(typeId);
		if (it == m_typeVectors.end()) {
			auto* vec = new std::vector<T>();
			m_typeVectors[typeId] = vec;
			m_ranges.emplace_back(std::make_unique<GameObjectRangeImpl<T>>(vec));
			return *vec;
		}
		return *static_cast<std::vector<T>*>(m_typeVectors[typeId]);
	}

	void _deleteVector(void* ptr) {
		delete ptr; // for now
	}

	void _emplaceBaseSystems(entt::entity entity, std::string& name);

public:
	using iterator = GameObjectSystemIterator;
	~GameObjectSystem() {
		for (auto& [_, vecPtr] : m_typeVectors)
			_deleteVector(vecPtr);
	}
	GameObjectSystem() = delete;
	GameObjectSystem(uint32_t sceneIndex, entt::registry* const registry)
		: SystemECS{ registry }, m_sceneIndex{sceneIndex}
	{}

	iterator begin() { return iterator(&m_ranges, 0, 0); }
	iterator end() { return iterator(&m_ranges, m_ranges.size(), 0); }

	template<typename T, typename... Args>
	T& createGameObject(std::string& name, Args&&... args) {
		auto& vec = _getVectorForType<T>();
		vec.emplace_back(std::forward<Args>(args)...);
		T& obj = vec.back();
		obj.m_entity = m_reg->create();
		obj.m_reg = m_reg;
		_emplaceBaseSystems(obj.m_entity, name);
		return obj;
	}

	template<typename T>
	std::vector<T>& getAllOfType() {
		return getVectorForType<T>();
	}


};

#endif