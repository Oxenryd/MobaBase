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
    float m_camSpeed = 12.5f;


public:
    GameScene(size_t arenaSize, uint32_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
        
        m_go = gameObjectSystem().createGameObject<GameObject>(m_name);
        MeshDescription meshInfo{};
        const std::string path = std::format("{}{}", ASSETS_DIR, "crytek-sponza-hd/sponza.obj"); //"Cube/cube.obj" //"crytek-sponza-hd/sponza.obj"

        Mesh modelMesh{};
        sceneRender().createMeshFromModel(path, &modelMesh, &m_go);

        sceneRender().submitPersistent(m_go, meshInfo.meshIndex, 512);
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


        Engine::getInstance()->getInputManager()->
            onMouseRightDown.subscribe([](MouseState state) -> void {
            Engine::getInstance()->getInputManager()->enableRelativeMouse();
                                       });
        Engine::getInstance()->getInputManager()->
            onMouseRightUp.subscribe([](MouseState state) -> void {
            Engine::getInstance()->getInputManager()->disableRelativeMouse();
                                     });

        Engine::getInstance()->getInputManager()->
            onMouseRightHold.subscribe([this](MouseState state) -> void 
                                       {
            auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
            auto dt = Timing::deltaTimeF();
            cam->rotateLocal({ -(float)state.lastPositionDelta.x * dt, -(float)state.lastPositionDelta.y * dt, 0.0f });
                                       });

    }

    void update(float dt) {

    }

    void lateUpdate(float dt) {
        //setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif