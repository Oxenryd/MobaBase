#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "Scene.h"
#include "PSO.hpp"

class GameScene : public Scene<GameScene>
{
public:
    static SceneBase* createDefault(void* arg) {
        return new DefaultScene{};
    }
};


#endif