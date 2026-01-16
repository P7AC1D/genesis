#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Genesis {

    enum class LightType {
        Directional,
        Point,
        Spot
    };

    struct Light {
        LightType Type = LightType::Directional;
        
        glm::vec3 Position{0.0f, 10.0f, 0.0f};    // For point/spot lights
        glm::vec3 Direction{-0.2f, -1.0f, -0.3f}; // For directional/spot lights
        glm::vec3 Color{1.0f, 0.95f, 0.9f};       // Light color
        float Intensity = 1.0f;                    // Light intensity
        
        // Attenuation (for point/spot lights)
        float Constant = 1.0f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
        
        // Spot light parameters
        float InnerCutoff = 12.5f;  // degrees
        float OuterCutoff = 17.5f;  // degrees
        
        bool Enabled = true;
    };

    struct LightingSettings {
        glm::vec3 AmbientColor{0.15f, 0.15f, 0.2f};
        float AmbientIntensity = 1.0f;
        
        // Fog settings for atmosphere
        glm::vec3 FogColor{0.7f, 0.8f, 0.9f};
        float FogDensity = 0.0f;  // 0 = no fog
        float FogStart = 50.0f;
        float FogEnd = 200.0f;
    };

    // Maximum lights supported in shader
    constexpr int MAX_POINT_LIGHTS = 4;

    class LightManager {
    public:
        LightManager();
        ~LightManager() = default;

        // Directional light (sun)
        void SetDirectionalLight(const Light& light);
        Light& GetDirectionalLight() { return m_DirectionalLight; }
        const Light& GetDirectionalLight() const { return m_DirectionalLight; }

        // Point lights
        void AddPointLight(const Light& light);
        void RemovePointLight(int index);
        void ClearPointLights();
        Light& GetPointLight(int index) { return m_PointLights[index]; }
        const std::vector<Light>& GetPointLights() const { return m_PointLights; }
        int GetPointLightCount() const { return static_cast<int>(m_PointLights.size()); }

        // Ambient/global settings
        void SetAmbientColor(const glm::vec3& color);
        void SetAmbientIntensity(float intensity);
        glm::vec3 GetAmbientColor() const { return m_Settings.AmbientColor; }
        float GetAmbientIntensity() const { return m_Settings.AmbientIntensity; }

        // Fog
        void SetFogEnabled(bool enabled) { m_Settings.FogDensity = enabled ? 0.02f : 0.0f; }
        void SetFogColor(const glm::vec3& color) { m_Settings.FogColor = color; }
        void SetFogDensity(float density) { m_Settings.FogDensity = density; }

        const LightingSettings& GetSettings() const { return m_Settings; }
        LightingSettings& GetSettings() { return m_Settings; }

        // Time-of-day simulation
        void SetTimeOfDay(float hours); // 0-24
        float GetTimeOfDay() const { return m_TimeOfDay; }

    private:
        void UpdateSunFromTimeOfDay();

        Light m_DirectionalLight;
        std::vector<Light> m_PointLights;
        LightingSettings m_Settings;
        float m_TimeOfDay = 12.0f; // Noon by default
    };

}
