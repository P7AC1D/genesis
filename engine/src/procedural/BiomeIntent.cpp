#include "genesis/procedural/BiomeIntent.h"

namespace Genesis
{

    ClimateSettings BiomeIntentMapper::DeriveSettings(const BiomeIntent &intent)
    {
        ClimateSettings settings;

        DeriveTemperature(intent, settings);
        DerivePrecipitation(intent, settings);
        DeriveVegetation(intent, settings);
        DeriveClimateWarping(intent, settings);

        return settings;
    }

    void BiomeIntentMapper::DeriveTemperature(const BiomeIntent &intent,
                                              ClimateSettings &settings)
    {
        // temperatureBias directly controls base temperature
        // 0 = cold (0.1), 1 = hot (0.9)
        settings.baseTemperature = Lerp(0.1f, 0.9f, intent.temperatureBias);

        // temperatureRange controls amplitude of variation
        // 0 = uniform (0.1), 1 = extreme (0.8)
        settings.temperatureAmplitude = Lerp(0.1f, 0.8f, intent.temperatureRange);

        // climateScale affects temperature noise frequency
        // Larger scale = lower frequency = bigger climate zones
        settings.temperatureFrequency = Lerp(0.02f, 0.005f, intent.climateScale);

        // Latitude influence scales with temperature range
        settings.latitudeInfluence = Lerp(0.2f, 0.8f, intent.temperatureRange);

        // Elevation lapse rate is relatively constant
        // Slightly affected by aridity (dry air = stronger lapse rate)
        settings.elevationLapseRate = Lerp(0.005f, 0.008f, intent.aridity);
    }

    void BiomeIntentMapper::DerivePrecipitation(const BiomeIntent &intent,
                                                ClimateSettings &settings)
    {
        // humidity directly controls base precipitation
        // 0 = dry (0.1), 1 = wet (0.9)
        settings.basePrecipitation = Lerp(0.1f, 0.9f, intent.humidity);

        // Variation inversely affected by humidity (wet worlds more uniform)
        settings.precipitationVariation = Lerp(0.5f, 0.2f, intent.humidity);

        // Precipitation frequency follows climate scale
        settings.precipitationFrequency = Lerp(0.03f, 0.008f, intent.climateScale);

        // aridity controls evaporation and water retention
        // High aridity = fast evaporation, low retention
        settings.evaporationRate = Lerp(0.1f, 0.8f, intent.aridity);
        settings.waterRetention = Lerp(0.8f, 0.2f, intent.aridity);
    }

    void BiomeIntentMapper::DeriveVegetation(const BiomeIntent &intent,
                                             ClimateSettings &settings)
    {
        // fertility affects vegetation threshold and density
        // High fertility = lower moisture needed, denser vegetation
        settings.vegetationThreshold = Lerp(0.5f, 0.1f, intent.fertility);
        settings.vegetationDensity = Lerp(0.2f, 1.0f, intent.fertility);
    }

    void BiomeIntentMapper::DeriveClimateWarping(const BiomeIntent &intent,
                                                 ClimateSettings &settings)
    {
        // chaos controls climate zone distortion
        settings.climateWarpStrength = Lerp(0.05f, 0.5f, intent.chaos);
        settings.climateWarpScale = Lerp(0.3f, 1.0f, intent.chaos);
    }

}
