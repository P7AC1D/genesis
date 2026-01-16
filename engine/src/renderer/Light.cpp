#include "genesis/renderer/Light.h"
#include <cmath>
#include <algorithm>

namespace Genesis {

    LightManager::LightManager() {
        // Default sun light
        m_DirectionalLight.Type = LightType::Directional;
        m_DirectionalLight.Direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
        m_DirectionalLight.Color = glm::vec3(1.0f, 0.95f, 0.9f);
        m_DirectionalLight.Intensity = 1.0f;
        m_DirectionalLight.Enabled = true;
        
        m_PointLights.reserve(MAX_POINT_LIGHTS);
    }

    void LightManager::SetDirectionalLight(const Light& light) {
        m_DirectionalLight = light;
        m_DirectionalLight.Type = LightType::Directional;
    }

    void LightManager::AddPointLight(const Light& light) {
        if (m_PointLights.size() < MAX_POINT_LIGHTS) {
            Light pointLight = light;
            pointLight.Type = LightType::Point;
            m_PointLights.push_back(pointLight);
        }
    }

    void LightManager::RemovePointLight(int index) {
        if (index >= 0 && index < static_cast<int>(m_PointLights.size())) {
            m_PointLights.erase(m_PointLights.begin() + index);
        }
    }

    void LightManager::ClearPointLights() {
        m_PointLights.clear();
    }

    void LightManager::SetAmbientColor(const glm::vec3& color) {
        m_Settings.AmbientColor = color;
    }

    void LightManager::SetAmbientIntensity(float intensity) {
        m_Settings.AmbientIntensity = intensity;
    }

    void LightManager::SetTimeOfDay(float hours) {
        m_TimeOfDay = std::fmod(hours, 24.0f);
        if (m_TimeOfDay < 0) m_TimeOfDay += 24.0f;
        UpdateSunFromTimeOfDay();
    }

    void LightManager::UpdateSunFromTimeOfDay() {
        // Convert time to sun angle
        // 6:00 = sunrise (east), 12:00 = noon (overhead), 18:00 = sunset (west)
        float sunAngle = (m_TimeOfDay - 6.0f) / 12.0f * 3.14159f; // 0 to PI from 6am to 6pm
        
        // Calculate sun direction
        float height = std::sin(sunAngle);
        float horizontal = std::cos(sunAngle);
        
        // Sun moves from east (+X) to west (-X)
        m_DirectionalLight.Direction = glm::normalize(glm::vec3(-horizontal, -std::abs(height), -0.3f));
        
        // Adjust color based on time of day
        if (m_TimeOfDay < 6.0f || m_TimeOfDay > 20.0f) {
            // Night - moonlight
            m_DirectionalLight.Color = glm::vec3(0.2f, 0.2f, 0.4f);
            m_DirectionalLight.Intensity = 0.2f;
            m_Settings.AmbientColor = glm::vec3(0.05f, 0.05f, 0.1f);
            m_Settings.AmbientIntensity = 1.0f;
        } else if (m_TimeOfDay < 7.0f || m_TimeOfDay > 19.0f) {
            // Dawn/dusk - orange/red
            float t = (m_TimeOfDay < 12.0f) ? (m_TimeOfDay - 6.0f) : (20.0f - m_TimeOfDay);
            m_DirectionalLight.Color = glm::vec3(1.0f, 0.6f + t * 0.35f, 0.3f + t * 0.6f);
            m_DirectionalLight.Intensity = 0.5f + t * 0.5f;
            m_Settings.AmbientColor = glm::vec3(0.2f, 0.15f + t * 0.05f, 0.1f + t * 0.1f);
            m_Settings.AmbientIntensity = 1.0f;
        } else {
            // Day
            float noonFactor = 1.0f - std::abs(m_TimeOfDay - 12.0f) / 6.0f;
            m_DirectionalLight.Color = glm::vec3(1.0f, 0.95f + noonFactor * 0.05f, 0.9f + noonFactor * 0.1f);
            m_DirectionalLight.Intensity = 0.8f + noonFactor * 0.2f;
            m_Settings.AmbientColor = glm::vec3(0.15f + noonFactor * 0.1f, 0.15f + noonFactor * 0.1f, 0.2f + noonFactor * 0.05f);
            m_Settings.AmbientIntensity = 1.0f;
        }
    }

}
