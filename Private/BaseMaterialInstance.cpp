//
// Created by oxenryd on 2026-04-26.
//

#include "HlslTypes.h"
#include "Engine.h"
#include "VulkanContext.hpp"

void BaseMaterialInstance::setTextureFlags(uint32_t flags) {
    textures.flags = flags;
    auto* vkCtx = Engine::getInstance()->getVulkanContext();
    vkCtx->pendingMaterialUpdate = vkCtx->pendingMaterialUpdate & VulkanContext::MatUpdatePendingState::Refresh;
}
