#ifndef MELONDSANDROIDCONFIGURATION_H
#define MELONDSANDROIDCONFIGURATION_H

#include "Configuration.h"
#include "MelonDS.h"

namespace MelonDSAndroidConfiguration {
    MelonDSAndroid::EmulatorConfiguration buildEmulatorConfiguration(JNIEnv* env, jobject emulatorConfiguration);
    MelonDSAndroid::FirmwareConfiguration buildFirmwareConfiguration(JNIEnv* env, jobject firmwareConfiguration);
    std::unique_ptr<MelonDSAndroid::RenderSettings> buildRenderSettings(JNIEnv* env, MelonDSAndroid::Renderer renderer, jobject renderSettings);
    bool getEmuLnkEnabled(JNIEnv* env, jobject emulatorConfiguration);
    int getEmuLnkPort(JNIEnv* env, jobject emulatorConfiguration);
}

#endif //MELONDSANDROIDCONFIGURATION_H
