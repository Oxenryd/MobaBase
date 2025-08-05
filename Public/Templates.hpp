#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"
#include "Timing.h"
#include "Engine.h" // DELETE DELETE

class GameScene : public Scene<GameScene>
{
private:
    float m_time = 0;
    uint64_t m_drawHash{ 0 };
    std::string m_name{ "TestObject" };
    GameObject m_go;
    uint32_t m_camIndex;
    float m_camSpeed = 7.5f;


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
        m_camIndex = sceneRender().addCamera();
        Engine::getInstance()->setMainCamera(m_sceneIndex, m_camIndex);

        Engine::getInstance()->getInputManager()->onKeyHold.subscribe( [this](KeyCode code) -> void
                                                                    {
                                                                           auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
                                                                           auto dt = Timing::deltaTimeF();
                                                                           switch (code) {
                                                                               case KeyCode::W:
                                                                                   cam->translate(cam->transform().forward() * m_camSpeed * dt); break;
                                                                               case KeyCode::S:
                                                                                   cam->translate(-cam->transform().forward() * m_camSpeed * dt); break;
                                                                               case KeyCode::A:
                                                                                   cam->translate(-cam->transform().right() * m_camSpeed * dt); break;
                                                                               case KeyCode::D:
                                                                                   cam->translate(cam->transform().right() * m_camSpeed * dt); break;
                                                                               case KeyCode::Q:
                                                                                   cam->translate(cam->transform().up() * m_camSpeed * dt); break;
                                                                               case KeyCode::E:
                                                                                   cam->translate(-cam->transform().up() * m_camSpeed * dt); break;
                                                                           }
                                                                           
                                                                       });
    }

    void update(float dt) {
        m_time += dt;

        auto cos = std::cos(m_time);
        auto sin = std::sin(m_time);

        float rotSize = 1.75f;
        auto transform = m_go.transform();

        
        auto& mState = Engine::getInstance()->getInputManager()->currentMouseState();
        auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
        std::cout << std::format("\n mPos: {},{}\twhDelta: {},{}\tpDelta: {},{}\tbState: {}",
                                 mState.relativePosition.x,
                                 mState.relativePosition.y,
                                 mState.wheel.x,
                                 mState.wheel.y,
                                 mState.lastPositionDelta.x,
                                 mState.lastPositionDelta.y,
                                 mState.buttonState.getField());

        if (mState.buttonState.hasFlag(MouseButton::Right)) {
            
            glm::vec3 rotation{};
            cam->rotateLocal({ -(float)mState.lastPositionDelta.x * dt, -(float)mState.lastPositionDelta.y * dt, 0.0f });
        }        
    }

    void lateUpdate(float dt) {
        //setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif