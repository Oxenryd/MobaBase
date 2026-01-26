#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"
#include "Timing.h"
#include "Engine.h" // DELETE DELETE

class GameScene final : public Scene<GameScene>
{
private:
    float m_time = 0;
    uint64_t m_drawHash{ 0 };
    GameObject m_go1, m_go2, m_go3;
    entt::entity m_skyLight;
    uint32_t m_camIndex{};
    float m_camSpeed = 12.5f;
    std::vector<GameObject> m_goList;
    std::vector<float> m_startScales;
    std::vector<float> m_radSpeeds;

public:
    GameScene(size_t arenaSize, uint16_t sceneIndex)
        : Scene<GameScene>{ arenaSize, sceneIndex} {}
    static SceneBase* createDefault(size_t arenaSize, uint16_t index, void* arg) {
        return new GameScene{ arenaSize, index};
    }

    void start() {
       
        m_camIndex = sceneRender().addCamera();
        Engine::getInstance()->setMainCamera(m_sceneIndex, m_camIndex);
        auto camTrans = Engine::getInstance()->mainCamera()->transform();
        camTrans.modifyPosition() = glm::vec3{0,3, 30};


        auto dirLight = LightFactory::Directional(glm::vec3{-1, -0.85, -0.25});
        dirLight.positionVS[0] = 0;
        dirLight.positionVS[1] = 20;
        dirLight.positionVS[2] = 0;
        m_skyLight = m_reg.create();
        [[maybe_unused]] auto& lightRef = lightSystem().registerLight(dirLight, m_skyLight);

        //m_go1 = gameObjectSystem().createGameObject<GameObject>("MerryGoRound");       
        //size_t numOfObjects = 4096 * 64;
        //m_goList.reserve(numOfObjects);
        //auto step = MMath::fTAU / numOfObjects;
        //const std::string pathObject = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        //for (size_t i = 0; i < numOfObjects; ++i) {
        //    m_goList.emplace_back(gameObjectSystem().createGameObject<GameObject>(std::format("Cube_{}", i)));
        //    Mesh objectMesh{};
        //    sceneRender().createMeshFromModel(pathObject, &objectMesh, m_goList.back().entity());
        //    auto trans = objectMesh.getTransform();
        //    trans.modifyPosition() = glm::vec3{std::cos(step * i) * 12, std::sin(step * i) * 12, 0};
        //    trans.modifyRotation() = MRandom::nextRotation();
        //    auto scale = MRandom::nextFloat(0.3f, 1.6f);
        //    m_startScales.push_back(scale);
        //    trans.modifyScale() = glm::vec3{ scale ,scale ,scale };
        //    m_radSpeeds.push_back(MRandom::nextFloat(1.3f, 8.f));
        //    

        //    m_goList.back().transform().setParent(m_go1);
        //}

        
        //m_go2 = gameObjectSystem().createGameObject<GameObject>("Box");
        //m_go3 = gameObjectSystem().createGameObject<GameObject>("Box");

        // "Cube/cube.obj" "crytek-sponza-hd/sponza.obj" "SmallRoom/smallRoom_mirror_window.obj"    "Sphere/sphere.obj"
        m_go1 = gameObjectSystem().createGameObject<GameObject>("Model1");
        const std::string path1 = std::format("{}{}", ASSETS_DIR, "crytek-sponza-hd/sponza.obj");
        Mesh modelMesh1{};
        sceneRender().createMeshFromModel(path1, &modelMesh1, m_go1.entity());
        float volume = 0.0f;
        entt::entity largestSubEntity = entt::null;
        const auto subMeshes = modelMesh1.getSubmeshes();
        for (const auto& subMesh : subMeshes) {
            BoundingVolume bVol{&registry(), subMesh.entity};
            auto aabb = bVol.getCoarseAABB();
            if (aabb.volume() > volume) {
                volume = aabb.volume();
                largestSubEntity = subMesh.entity;
            }
        }
        if (largestSubEntity != entt::null)
            registry().get<BoundingVolumeComponent>(largestSubEntity).flags = static_cast<uint32_t>(BoundingVolumeFlags::Occluder);
        
        
        //const std::string path2 = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        //Mesh modelMesh2{};
        //sceneRender().createMeshFromModel(path2, &modelMesh2, m_go2.entity());
        //for (auto& subMesh : modelMesh2.getSubmeshes()) {
        //    BoundingVolume bVol{ &registry(), subMesh.entity };
        //    bVol.setFlags(BoundingVolumeFlags::Occluder);
        //}

        //const std::string path3 = std::format("{}{}", ASSETS_DIR, "Cube/cube.obj");
        //Mesh modelMesh3{};
        //sceneRender().createMeshFromModel(path3, &modelMesh3, m_go3.entity());

        //modelMesh3.getTransform().modifyPosition() = { -12, 0, 0 };
        //modelMesh2.getTransform().modifyPosition() = { -12, 0, 0 };
        //auto& reg = registry();

        InputManager::onKeyHold().subscribe( [this](const KeyCode code) -> void
                       {
                           const auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
                           const auto dt = Timing::deltaTimeF();
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
                               default: break;
                           }
                              
                       });


        InputManager::onKeyDown().subscribe([this](const KeyCode code) -> void
               {
                   switch (code) {
                       case KeyCode::B: sceneRender().setDrawAABBs(!sceneRender().drawCoarseAbbs()); break;
                       case KeyCode::O: sceneRender().setDrawOccluders(!sceneRender().drawOccluders()); break;
                       case KeyCode::N: sceneRender().setDrawNodes(!sceneRender().drawNodes()); break;
                       case KeyCode::ArrowLeft:
                       {
                           auto sTrans = Transform{ &registry(), m_go2};
                           [[maybe_unused]] auto& pos = sTrans.modifyScale() -= glm::vec3{ 1,0,0 };

                       } break;
                       case KeyCode::ArrowRight:
                       {
                           auto sTrans = Transform{ &registry(), m_go2 };
                           [[maybe_unused]] auto& pos = sTrans.modifyScale() += glm::vec3{ 1,0,0 };

                       } break;
                       case KeyCode::ArrowUp:
                       {
                           auto sTrans = Transform{ &registry(), m_go2 };
                           [[maybe_unused]] auto& pos = sTrans.modifyScale() += glm::vec3{ 0,1,0 };

                       } break;
                       case KeyCode::ArrowDown:
                       {
                           auto sTrans = Transform{ &registry(), m_go2 };
                           [[maybe_unused]] auto& pos = sTrans.modifyScale() -= glm::vec3{ 0,1,0 };

                       } break;

                       case KeyCode::T:
                       {
                           Engine::getInstance()->armFrameTrace();
                       } break;
                       default: break;
                   }
               });


        InputManager::onMouseDown().subscribe([](const MouseState& state, const MouseButton button) {
                                        if (button == MouseButton::Right) {
                                            InputManager::setCursorMode(CursorMode::HiddenGrabbed);
                                        }
                                    });

        InputManager::onMouseHold().subscribe([this](const MouseState& state, const MouseButton button) {
            if (button == MouseButton::Right) {
                const auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
                auto dt = Timing::deltaTime();
                const auto deltaPos = state.deltaPosition;
                //auto height = Engine::getInstance()->getWndSurface()->height;
                //auto width = Engine::getInstance()->getWndSurface()->width;
                //cam->rotateLocal(Input::scaledMouseMovementVec3(state, dt, 0.1f, {width, height}));
                cam->rotateLocal(static_cast<float>(dt) *
                    glm::vec3{-state.deltaPosition.x, -state.deltaPosition.y, 0.0f});
            }
        });

        InputManager::onMouseUp().subscribe([](const MouseState& state, const MouseButton button) {
            if (button == MouseButton::Right) {
                InputManager::setCursorMode(CursorMode::Normal);
            }
        });

        // InputManager::
        //     onMouseRightDown.subscribe([](MouseState state) -> void {
        //
        // Engine::getInstance()->getInputManager()->
        //     onMouseRightUp.subscribe([](MouseState state) -> void {
        //     Engine::getInstance()->getInputManager()->disableRelativeMouse();
        //                              });
        //
        // Engine::getInstance()->getInputManager()->
        //     onMouseRightHold.subscribe([this](MouseState state) -> void
        //                                {
        //     auto cam = Camera::getCamera(m_sceneIndex, m_camIndex);
        //     auto dt = Timing::deltaTime();
        //     auto height = Engine::getInstance()->getWndSurface()->height;
        //     auto width = Engine::getInstance()->getWndSurface()->width;
        //     cam->rotateLocal(Input::scaledMouseMovementVec3(state, dt, 0.1f, {width, height}));
        //                               });


        //Engine::getInstance()->getGlobalSystem().printAllTags();
        //transformSystem().printHierarchy();
    }

    void update(double dt) {

        return;

        m_time += Timing::deltaTimeF();

        auto addPos = glm::vec3{0.0025f, 0, 0};//glm::vec3{std::cos(m_time), std::sin(m_time), 0};

        auto merryTrans = Transform{ &m_reg, m_go1 };
        auto children = merryTrans.getChildrenEntities();
        merryTrans.rotate(glm::vec3{ 0.0f, 0.5f, 1.0f } * Timing::deltaTimeF());

        auto view = m_reg.view<TransformComponent>();
        {
            PROFILE_SCOPE("TRANSFORM_UPDATE");
            MWork::for_loop(0, children.size(), 16,
                            [&](std::size_t i) {

                                auto& transComp = view.get<TransformComponent>(children[i]);
                                auto& rotation = transformSystem().rotations()[transComp.dataIndex];
                                Transform::rotateLocal(rotation, glm::vec3{ 0.0f, 1.0f, 0.0f } *Timing::deltaTimeF());

                                auto cos = std::cos(m_radSpeeds[i] * m_time) * m_startScales[i] * 0.66f;
                                auto curScale = m_startScales[i] + cos;
                                auto& scale = transformSystem().scales()[transComp.dataIndex];
                                scale = glm::vec3{ curScale };

                                transComp.state.setByEnum(ObjectState::DirtyTransform);
                                transComp.state.setByEnum(ObjectState::ScaleDirty);
                                transComp.state.setByEnum(ObjectState::RotationDirty);
                            });
        }
    }

    void lateUpdate(double dt) {
        //setUnload();
    }

    void unload() {
        sceneRender().cancelPersistentDraw(m_drawHash);
    }
};


#endif