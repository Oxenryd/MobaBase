#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"

class GameScene : public Scene<GameScene>
{
public:
    GameScene(uint32_t sceneIndex)
        : Scene<GameScene>{sceneIndex} {}
    static SceneBase* createDefault(uint32_t sceneindex, void* arg) {
        return new GameScene{0};
    }

    void start() {
        std::string name{ "TestObject" };
        auto& go = gameObjectSystem().createGameObject<GameObject>(name);
    }

    void update(float dt) {
        
    }

};


#endif