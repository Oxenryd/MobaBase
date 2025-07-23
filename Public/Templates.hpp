#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"

class GameScene : public Scene<GameScene>
{
private:
    float m_time = 0;
    std::string m_name{ "TestObject" };
    GameObject m_go;

public:
    GameScene(uint32_t sceneIndex)
        : Scene<GameScene>{sceneIndex} {}
    static SceneBase* createDefault(uint32_t index, void* arg) {
        return new GameScene{index};
    }

    void start() {
        
        m_go = gameObjectSystem().createGameObject<GameObject>(m_name);
    }

    void update(float dt) {
        m_time += dt;

        auto transform = m_go.transform();
        transform.modifyPosition() = glm::vec3{5 * std::cos(m_time), 5 * std::sin(m_time), 0.0};
        //auto& pos = transform.position();
        //LOGLINE(LogType::Info, LogMod::Engine, std::format("Position: {}, {}", pos.x, pos.y));
    }

};


#endif