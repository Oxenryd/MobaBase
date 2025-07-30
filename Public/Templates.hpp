#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"

class GameScene : public Scene<GameScene>
{
private:
    float m_time = 0;
    uint64_t m_drawHash{ 0 };
    std::string m_name{ "TestObject" };
    GameObject m_go;

public:
    GameScene(size_t arenaSize, uint32_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
        
        m_go = gameObjectSystem().createGameObject<GameObject>(m_name);
        MeshDescription meshInfo{};
        const std::string path = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        auto ec = sceneRender().loadModel(path, &meshInfo);
        if (!EC_FAILED(ec)) {
            MeshComponent meshComp{};
            meshComp.meshIndex = meshInfo.meshIndex;
            registry().emplace<MeshComponent>(m_go, meshComp);
        }

        sceneRender().submitPersistent(m_go, meshInfo.meshIndex, 0, 512);
    }

    void update(float dt) {
        m_time += dt;

        auto transform = m_go.transform();
        transform.modifyPosition() = glm::vec3{5 * std::cos(m_time), 5 * std::sin(m_time), 0.0};

    }

    void lateUpdate(float dt) {
        //setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif