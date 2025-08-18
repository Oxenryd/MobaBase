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
    GameObject m_go1, m_go2, m_go3;
    entt::entity m_skyLight;
    uint32_t m_camIndex{};
    float m_camSpeed = 12.5f;
    std::vector<GameObject> m_goList;


public:
    GameScene(size_t arenaSize, uint32_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint32_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
       
        auto dirLight = LightFactory::Directional(glm::vec3{-1, -0.85, -0.25});
        dirLight.positionVS = glm::vec3{0, 20, 0};
        m_skyLight = m_reg.create();
        auto& lightRef = lightSystem().registerLight(dirLight, m_skyLight);

        
        size_t numOfObjects = 4;
        m_goList.reserve(numOfObjects);
        auto step = MMath::fTAU / numOfObjects;
        const std::string pathObject = std::format("{}{}", ASSETS_DIR, "Sphere/sphere.obj");
        for (size_t i = 0; i < numOfObjects; ++i) {
            m_goList.emplace_back(gameObjectSystem().createGameObject<GameObject>(std::format("Object_{}", i)));
            Mesh objectMesh{};
            sceneRender().createMeshFromModel(pathObject, &objectMesh, m_goList[i].entity());
            auto trans = objectMesh.getTransform();
            trans.modifyPosition() = glm::vec3{std::cos(step * i) * 12, std::sin(step * i) * 12, 0};
        }

        m_go1 = gameObjectSystem().createGameObject<GameObject>("Room");
        m_go2 = gameObjectSystem().createGameObject<GameObject>("Box");
        //m_go3 = gameObjectSystem().createGameObject<GameObject>("Box");

        //"Cube/cube.obj" //"crytek-sponza-hd/sponza.obj" //"SmallRoom/smallRoom_mirror_window.obj"    //"Sphere/sphere.obj"
        const std::string path1 = std::format("{}{}", ASSETS_DIR, "SmallRoom/smallRoom_mirror_window.obj");
        Mesh modelMesh1{};
        sceneRender().createMeshFromModel(path1, &modelMesh1, m_go1.entity());
        float volume = 0.0f;
        entt::entity largestSubEntity = entt::null;
        for (auto& subMesh : modelMesh1.getSubmeshes()) {
            BoundingVolume bVol{&registry(), subMesh.entity};
            auto aabb = bVol.getCoarseAABB();
            if (aabb.volume() > volume) {
                volume = aabb.volume();
                largestSubEntity = subMesh.entity;
            }
        }
        if (largestSubEntity != entt::null)
            registry().get<BoundingVolumeComponent>(largestSubEntity).flags = static_cast<uint32_t>(BoundingVolumeFlags::Occluder);


        const std::string path2 = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        Mesh modelMesh2{};
        sceneRender().createMeshFromModel(path2, &modelMesh2, m_go2.entity());
        //for (auto& subMesh : modelMesh2.getSubmeshes()) {
        //    BoundingVolume bVol{ &registry(), subMesh.entity };
        //    bVol.setFlags(BoundingVolumeFlags::Occluder);
        //}

        //const std::string path3 = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        //Mesh modelMesh3{};
        //sceneRender().createMeshFromModel(path3, &modelMesh3, m_go3.entity());

        //modelMesh3.getTransform().modifyPosition() = { -12, 0, 0 };
        modelMesh2.getTransform().modifyPosition() = { -12, 0, 0 };

        auto& reg = registry();

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
                                          case KeyCode::ArrowLeft:
                                          {
                                              auto sTrans = Transform{&registry(), m_go2};
                                              auto& pos = sTrans.modifyScale() -= glm::vec3{ 1,0,0 };
                                             
                                          } break;
                                          case KeyCode::ArrowRight:
                                          {
                                              auto sTrans = Transform{ &registry(), m_go2 };
                                              auto& pos = sTrans.modifyScale() += glm::vec3{ 1,0,0 };
                                    
                                          } break;
                                          case KeyCode::ArrowUp:
                                          {
                                              auto sTrans = Transform{ &registry(), m_go2 };
                                              auto& pos = sTrans.modifyScale() += glm::vec3{ 0,1,0 };

                                          } break;
                                          case KeyCode::ArrowDown:
                                          {
                                              auto sTrans = Transform{ &registry(), m_go2 };
                                              auto& pos = sTrans.modifyScale() -= glm::vec3{ 0,1,0 };

                                          } break;

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


        Engine::getInstance()->getGlobalSystem().printAllTags();
        transformSystem().printHierarchy();
    }

    void update(float dt) {

        m_time += Timing::deltaTimeF();
        auto ballTransform = m_go2.transform();
        //ballTransform.setFromEuler({ m_time * 3, m_time * 25, 0});
        //ballTransform.modifyPosition() = { 12, 0, 0 };

        //auto boxTransform = m_go3.transform();
        //boxTransform.modifyPosition() = { Engine::sinF() * 10, Engine::cosF() * 10, 0};
        //boxTransform.modifyScale() = { (Engine::sinF() + 1) * 10, (Engine::cosF() + 1) * 10, 0 };
        //boxTransform.setFromEuler({ Engine::sinF() * 2, Engine::cosF() * 2, 0});

        

        for (size_t i = 0; i < m_goList.size(); ++i) {
            GameObject& go = m_goList[i];
            auto transform = Transform{ &m_reg, go.entity() };
            auto& pos = transform.modifyPosition();

            
            //pos += glm::vec3{Engine::cosF(), Engine::sinF(), 0};
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