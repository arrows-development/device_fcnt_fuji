/*
 * Copyright (C) 2018-2022 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Light.h"

#include <fstream>

#define LCD_LED "/sys/class/leds/lcd-backlight/"
#define MTKFB "/proc/mtkfb"

namespace {
constexpr uint32_t kMaxBrightness = 4080;
constexpr int kDefaultConnectorId = 34;

/*
 * Write value to path and close file.
 */
static bool set(const std::string& path, const std::string& value) {
    std::ofstream file(path);

    if (!file.is_open()) {
        LOG(ERROR) << "failed to write " << value << " to " << path;
        return false;
    }

    file << value;
    if (!file.good()) {
        LOG(ERROR) << "failed to write " << value << " to " << path;
        return false;
    }

    return true;
}

static int getConnectorId() {
    std::ifstream file(LCD_LED "connector_id");
    int connectorId;

    if (file >> connectorId) {
        return connectorId;
    }

    LOG(WARNING) << "failed to read DRM connector id, using " << kDefaultConnectorId;
    return kDefaultConnectorId;
}

static uint32_t getBrightness(const HwLightState& state) {
    uint32_t alpha, red, green, blue;

    /*
     * Extract brightness from AARRGGBB.
     */
    alpha = (state.color >> 24) & 0xFF;
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    /*
     * Scale RGB brightness using Alpha brightness.
     */
    red = red * alpha / 0xFF;
    green = green * alpha / 0xFF;
    blue = blue * alpha / 0xFF;

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

static inline uint32_t scaleBrightness(uint32_t brightness) {
    return brightness * kMaxBrightness / 0xFF;
}

static inline uint32_t getScaledBrightness(const HwLightState& state) {
    return scaleBrightness(getBrightness(state));
}

static void handleBacklight(const HwLightState& state) {
    const uint32_t brightness = getScaledBrightness(state);
    const int connectorId = getConnectorId();

    const std::string command =
            "conn_backlight:" + std::to_string(connectorId) + "," + std::to_string(brightness);
    if (set(MTKFB, command)) {
        set(LCD_LED "brightness", std::to_string(brightness));
    }
}

static std::vector<LightType> backends = {
    LightType::BACKLIGHT,
};

}  // anonymous namespace

namespace aidl {
namespace android{
namespace hardware {
namespace light {

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    switch(id) {
        case (int) LightType::BACKLIGHT:
            handleBacklight(state);
            return ndk::ScopedAStatus::ok();
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    int i = 0;

    for (const LightType& backend : backends) {
        HwLight hwLight;
        hwLight.id = (int) backend;
        hwLight.type = backend;
        hwLight.ordinal = i;
        lights->push_back(hwLight);
        i++;
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
