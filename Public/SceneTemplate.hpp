#ifndef SCENETEMPLATE_HPP
#define SCENETEMPLATE_HPP

#include "Scene.h"

class GameScene : public Scene<GameScene>
{
public:
    static SceneBase* createDefault(void* arg) {
        return new DefaultScene{};
    }



};

#endif