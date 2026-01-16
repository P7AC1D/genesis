#pragma once

#include <cstdint>
#include <string>

namespace Genesis {

    class Scene;

    class Entity {
    public:
        Entity() = default;
        Entity(uint64_t id, Scene* scene);
        Entity(const Entity& other) = default;

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        const T& GetComponent() const;

        template<typename T>
        bool HasComponent() const;

        template<typename T>
        void RemoveComponent();

        bool IsValid() const { return m_EntityID != 0 && m_Scene != nullptr; }
        uint64_t GetID() const { return m_EntityID; }

        operator bool() const { return IsValid(); }
        operator uint64_t() const { return m_EntityID; }

        bool operator==(const Entity& other) const {
            return m_EntityID == other.m_EntityID && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const {
            return !(*this == other);
        }

    private:
        uint64_t m_EntityID = 0;
        Scene* m_Scene = nullptr;
    };

}
