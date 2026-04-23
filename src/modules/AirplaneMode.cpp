#include "AirplaneMode.h"

#include "NodeDB.h"
#include "configuration.h"
#include "main.h"

#ifdef ARCH_ESP32
#include <Preferences.h>
#endif

// Shared with main-esp32.cpp's reboot counter namespace.
static constexpr const char *kNvsNs = "meshtastic";
static constexpr const char *kKeyActive = "apm_active";
static constexpr const char *kKeyWifi = "apm_sv_wifi";
static constexpr const char *kKeyBt = "apm_sv_bt";
static constexpr const char *kKeyLora = "apm_sv_lora";

AirplaneMode &AirplaneMode::instance()
{
    static AirplaneMode singleton;
    return singleton;
}

#ifdef ARCH_ESP32
static bool readActiveFlag()
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, true)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(ro) failed; assuming inactive");
        return false;
    }
    bool active = prefs.isKey(kKeyActive) ? prefs.getBool(kKeyActive, false) : false;
    prefs.end();
    return active;
}

static void writeActiveFlag(bool active)
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, false)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(rw) failed; active flag WILL NOT persist");
        return;
    }
    prefs.putBool(kKeyActive, active);
    prefs.end();
}

static void savePreAirplaneState(bool wifi, bool bt, bool lora)
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, false)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(rw) failed; saved radio state WILL NOT persist");
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
    if (!prefs.begin(kNvsNs, true)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(ro) failed; cannot read saved radio state");
        return false;
    }
    bool have = prefs.isKey(kKeyWifi) && prefs.isKey(kKeyBt) && prefs.isKey(kKeyLora);
    if (have) {
        wifi = prefs.getBool(kKeyWifi, true);
        bt = prefs.getBool(kKeyBt, true);
        lora = prefs.getBool(kKeyLora, true);
    }
    prefs.end();
    return have;
}
#endif // ARCH_ESP32

bool AirplaneMode::isActive() const
{
#ifdef ARCH_ESP32
    return readActiveFlag();
#else
    return false;
#endif
}

void AirplaneMode::toggle()
{
#ifdef ARCH_ESP32
    bool turningOn = !isActive();
    LOG_WARN("AirplaneMode: %s", turningOn ? "entering" : "exiting");

    if (turningOn) {
        savePreAirplaneState(config.network.wifi_enabled, config.bluetooth.enabled, config.lora.tx_enabled);
        config.network.wifi_enabled = false;
        config.bluetooth.enabled = false;
        config.lora.tx_enabled = false;
        writeActiveFlag(true);
    } else {
        bool wifi = true, bt = true, lora = true;
        if (!loadPreAirplaneState(wifi, bt, lora))
            LOG_WARN("AirplaneMode: no saved pre-airplane state; defaulting all radios on");
        config.network.wifi_enabled = wifi;
        config.bluetooth.enabled = bt;
        config.lora.tx_enabled = lora;
        writeActiveFlag(false);
    }

    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    rebootAtMsec = millis() + 2000;
#else
    LOG_WARN("AirplaneMode: toggle unsupported on this platform (no NVS backend)");
#endif
}
