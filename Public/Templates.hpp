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
        uint32_t meshIndex{};
        auto ec = sceneRender().loadModel("H:\\Dev\\Projects\\MobaBase\\Assets\\Cube\\cube.obj", &meshIndex);
        if (!EC_FAILED(ec)) {
            MeshComponent meshComp{};
            meshComp.meshIndex = meshIndex;
            registry().emplace<MeshComponent>(m_go, meshComp);
        }

        DrawCommand dCmd{};
        dCmd.drawContextPtr = nullptr;
        dCmd.instanceRequest = false;
        dCmd.material = nullptr;
        dCmd.priority = 0.5f;
        dCmd.type = DrawType::Mesh;
        dCmd.persistent = true;
        m_drawHash = sceneRender().submitDraw(dCmd);
    }

    void update(float dt) {
        m_time += dt;

        auto transform = m_go.transform();
        transform.modifyPosition() = glm::vec3{5 * std::cos(m_time), 5 * std::sin(m_time), 0.0};

    }

    void lateUpdate(float dt) {
        setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif