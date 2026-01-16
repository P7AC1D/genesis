#include "genesis/ecs/Entity.h"
#include "genesis/ecs/Scene.h"

namespace Genesis {

    Entity::Entity(uint64_t id, Scene* scene)
        : m_EntityID(id), m_Scene(scene) {
    }

    // Template implementations would go here for a full ECS
    // For now, this is a placeholder for the skeleton

}
