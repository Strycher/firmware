#include "AirplaneMode.h"

#if HAS_SCREEN

#include "NodeDB.h"
#include "configuration.h"
#include "main.h"

#ifdef ARCH_ESP32
#include <Preferences.h>
#endif

// Shared with main-esp32.cpp's reboot counter namespace.
static constexpr const char *kNvsNs = "meshtastic";
static constexpr const char *kKeyWifi = "apm_sv_wifi";
static constexpr const char *kKeyBt = "apm_sv_bt";
static constexpr const char *kKeyLora = "apm_sv_lora";

AirplaneMode &AirplaneMode::instance()
{
    static AirplaneMode singleton;
    return singleton;
}

bool AirplaneMode::isActive() const
{
    // Derived from live config: if all three radios are disabled, we consider
    // the device in airplane mode regardless of how it got there (menu, CLI,
    // or phone app).
    return !config.network.wifi_enabled && !config.bluetooth.enabled && !config.lora.tx_enabled;
}

#ifdef ARCH_ESP32
static void savePreAirplaneState(bool wifi, bool bt, bool lora)
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, false)) {
        LOG_ERROR("AirplaneMode: Preferences.begin failed; saved state WILL NOT persist");
        return;
    }
    prefs.putBool(kKeyWifi, wifi);
    prefs.putBool(kKeyBt, bt);
    prefs.putBool(kKeyLora, lora);
    prefs.end();
}

static bool loadPreAirplaneState(bool &wifi, bool &bt, bool &lora)
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, true))
        return false;
    bool have = prefs.isKey(kKeyWifi) && prefs.isKey(kKeyBt) && prefs.isKey(kKeyLora);
    if (have) {
        wifi = prefs.getBool(kKeyWifi, true);
        bt = prefs.getBool(kKeyBt, true);
        lora = prefs.getBool(kKeyLora, true);
    }
    prefs.end();
    return have;
}
#endif

void AirplaneMode::toggle()
{
    bool turningOn = !isActive();
    LOG_WARN("AirplaneMode: %s", turningOn ? "entering" : "exiting");

    if (turningOn) {
#ifdef ARCH_ESP32
        savePreAirplaneState(config.network.wifi_enabled, config.bluetooth.enabled, config.lora.tx_enabled);
#endif
        config.network.wifi_enabled = false;
        config.bluetooth.enabled = false;
        config.lora.tx_enabled = false;
    } else {
        bool wifi = true, bt = true, lora = true;
#ifdef ARCH_ESP32
        if (!loadPreAirplaneState(wifi, bt, lora))
            LOG_WARN("AirplaneMode: no saved pre-airplane state; defaulting all radios on");
#endif
        config.network.wifi_enabled = wifi;
        config.bluetooth.enabled = bt;
        config.lora.tx_enabled = lora;
    }

    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    rebootAtMsec = millis() + 2000;
}

#endif // HAS_SCREEN
