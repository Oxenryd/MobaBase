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
    //std::string m_name1{ "TestObject" };
    GameObject m_go1, m_go2;
    entt::entity m_skyLight;
    uint32_t m_camIndex{};
    float m_camSpeed = 12.5f;


public:
    GameScene(size_t arenaSize, uint32_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
       
        m_skyLight = m_reg.create();
        m_reg.emplace<TransformComponent>(m_skyLight, TransformComponent{});
        auto dirLight = LightFactory::Directional(glm::vec3{-1, -0.85, -0.25});
        dirLight.positionVS = glm::vec3{0, 20, 0};
        auto& lightRef = lightSystem().registerLight(dirLight, m_skyLight);


        m_go1 = gameObjectSystem().createGameObject<GameObject>("Object1");
        m_go2 = gameObjectSystem().createGameObject<GameObject>("Object2");

        //"Cube/cube.obj" //"crytek-sponza-hd/sponza.obj" //"SmallRoom/smallRoom_mirror_window.obj"    //"Sphere/sphere.obj"
        const std::string path1 = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        Mesh modelMesh1{};
        sceneRender().createMeshFromModel(path1, &modelMesh1, m_go1.entity());
        for (auto& subMesh : modelMesh1.getSubmeshes()) {
            BoundingVolume bVol{&registry(), subMesh.entity};
            bVol.setFlags(BoundingVolumeFlags::Occluder);
        }

        const std::string path2 = std::format("{}{}", ASSETS_DIR, "Sphere/sphere.obj");
        Mesh modelMesh2{};
        sceneRender().createMeshFromModel(path2, &modelMesh2, m_go2.entity());
        for (auto& subMesh : modelMesh2.getSubmeshes()) {
            BoundingVolume bVol{ &registry(), subMesh.entity };
            bVol.setFlags(BoundingVolumeFlags::Occluder);
        }

        modelMesh2.getTransform().modifyPosition() = {0, 0, -5};


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
              onKeyDown.subscribe([this](KeyCode code) -> void
                                  {
                                      switch (code) {
                                          case KeyCode::B: sceneRender().setDrawAABBs(!sceneRender().drawCoarseAbbs()); break;
                                          case KeyCode::O: sceneRender().setDrawOccluders(!sceneRender().drawOccluders()); break;
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