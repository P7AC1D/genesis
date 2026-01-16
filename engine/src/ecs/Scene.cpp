#include "genesis/ecs/Scene.h"
#include "genesis/ecs/Entity.h"
#include "genesis/ecs/Components.h"
#include "genesis/renderer/Renderer.h"

namespace Genesis {

    Scene::Scene(const std::string& name)
        : m_Name(name) {
    }

    Scene::~Scene() {
        Clear();
    }

    Entity Scene::CreateEntity(const std::string& name) {
        uint64_t id = m_Registry.NextID++;
        m_Registry.Entities.push_back({id, true, name.empty() ? "Entity" : name});
        m_Components[id] = std::make_unique<EntityComponents>();
        return Entity(id, this);
    }

    void Scene::DestroyEntity(Entity entity) {
        auto it = std::find_if(m_Registry.Entities.begin(), m_Registry.Entities.end(),
            [&](const EntityData& e) { return e.ID == entity.GetID(); });
        
        if (it != m_Registry.Entities.end()) {
            m_Registry.Entities.erase(it);
            m_Components.erase(entity.GetID());
        }
    }

    void Scene::OnUpdate(float deltaTime) {
        // Update systems here
        // For now, just placeholder
    }

    void Scene::OnRender(Renderer& renderer) {
        // Find camera and render
        // For now, just placeholder
    }

    void Scene::Clear() {
        m_Registry.Entities.clear();
        m_Components.clear();
    }

    void Scene::CopyTo(Scene& other) {
        other.m_Name = m_Name + " (Copy)";
        // Deep copy entities and components
    }

    // EntityComponents storage (simple implementation)
    class EntityComponents {
    public:
        std::optional<TagComponent> Tag;
        std::optional<TransformComponent> Transform;
        std::optional<MeshRendererComponent> MeshRenderer;
        std::optional<CameraComponent> Camera;
        std::optional<DirectionalLightComponent> DirectionalLight;
        std::optional<PointLightComponent> PointLight;
        std::optional<ProceduralMeshComponent> ProceduralMesh;
        std::optional<RigidbodyComponent> Rigidbody;
        std::optional<BoxColliderComponent> BoxCollider;
    };

}
