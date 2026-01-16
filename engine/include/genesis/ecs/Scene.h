#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Genesis {

    class Entity;
    class Renderer;

    class Scene {
    public:
        Scene(const std::string& name = "Untitled Scene");
        ~Scene();

        Entity CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(Entity entity);

        void OnUpdate(float deltaTime);
        void OnRender(Renderer& renderer);

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        template<typename... Components>
        auto GetEntitiesWith() { return m_Registry.view<Components...>(); }

        // Scene management
        void Clear();
        void CopyTo(Scene& other);

        // Internal registry access (for ECS)
        auto& GetRegistry() { return m_Registry; }

    private:
        friend class Entity;

        std::string m_Name;
        
        // Simple entity storage - in production, use entt or similar
        struct EntityData {
            uint64_t ID;
            bool Active;
            std::string Name;
        };
        
        struct Registry {
            std::vector<EntityData> Entities;
            uint64_t NextID = 1;
            
            template<typename... Components>
            std::vector<uint64_t> view() { return {}; } // Placeholder
        };
        
        Registry m_Registry;
        std::unordered_map<uint64_t, std::unique_ptr<class EntityComponents>> m_Components;
    };

}
