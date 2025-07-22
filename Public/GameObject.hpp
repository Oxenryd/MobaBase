#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include <entt/entt.hpp>

class GameObjectSystem;
class GameObject
{
	friend GameObjectSystem;
private:
	
	entt::entity m_entity = entt::null;

public:
	virtual ~GameObject() {}
	GameObject() {}
};

#endif