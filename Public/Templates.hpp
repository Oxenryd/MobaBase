#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"

#include "Engine.h" // DELETE DELETE

class GameScene : public Scene<GameScene>
{
private:
    float m_time = 0;
    uint64_t m_drawHash{ 0 };
    std::string m_name{ "TestObject" };
    GameObject m_go;

    glm::vec3 m_camPos;
    glm::quat m_camRot;

public:
    GameScene(size_t arenaSize, uint32_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
        
        m_go = gameObjectSystem().createGameObject<GameObject>(m_name);
        MeshDescription meshInfo{};
        const std::string path = std::format("{}{}", ASSETS_DIR, "crytek-sponza-hd/sponza.obj");
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

        auto cos = std::cos(m_time);
        auto sin = std::sin(m_time);

        float rotSize = 1.75f;
        auto transform = m_go.transform();
        //transform.modifyPosition() = glm::vec3{ rotSize * cos, rotSize * sin, 0.0};
        //
        //transform.modifyRotation() = glm::quat( glm::vec3{0.0f, cos, 0.5f * sin });
        //
        //transform.modifyScale() = glm::vec3{0.5f * cos + 0.55f};

        
    }

    void lateUpdate(float dt) {
        //setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif