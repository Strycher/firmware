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
static constexpr const char *kKeySaveWifi = "apm_save_wifi";
static constexpr const char *kKeySaveBt = "apm_save_bt";
static constexpr const char *kKeySaveLora = "apm_save_lora";

// isActive() is called every frame from drawCommonHeader; cache the flag to
// avoid a blocking NVS read on each render. Initialized lazily on first
// access, invalidated/rewritten by toggle().
static bool s_cachedActive = false;
static bool s_cachedActiveLoaded = false;

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
    prefs.putBool(kKeySaveWifi, wifi);
    prefs.putBool(kKeySaveBt, bt);
    prefs.putBool(kKeySaveLora, lora);
    prefs.end();
}

static bool loadPreAirplaneState(bool &wifi, bool &bt, bool &lora)
{
    Preferences prefs;
    if (!prefs.begin(kNvsNs, true)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(ro) failed; cannot read saved radio state");
        return false;
    }
    bool have = prefs.isKey(kKeySaveWifi) && prefs.isKey(kKeySaveBt) && prefs.isKey(kKeySaveLora);
    if (have) {
        wifi = prefs.getBool(kKeySaveWifi, true);
        bt = prefs.getBool(kKeySaveBt, true);
        lora = prefs.getBool(kKeySaveLora, true);
    }
    prefs.end();
    return have;
}
#endif // ARCH_ESP32

bool AirplaneMode::isActive() const
{
#ifdef ARCH_ESP32
    if (!s_cachedActiveLoaded) {
        s_cachedActive = readActiveFlag();
        s_cachedActiveLoaded = true;
    }
    return s_cachedActive;
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
        s_cachedActive = true;
    } else {
        bool wifi = true, bt = true, lora = true;
        if (!loadPreAirplaneState(wifi, bt, lora))
            LOG_WARN("AirplaneMode: no saved pre-airplane state; defaulting all radios on");
        config.network.wifi_enabled = wifi;
        config.bluetooth.enabled = bt;
        config.lora.tx_enabled = lora;
        writeActiveFlag(false);
        s_cachedActive = false;
    }
    s_cachedActiveLoaded = true;

    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    rebootAtMsec = millis() + 2000;
#else
    LOG_WARN("AirplaneMode: toggle unsupported on this platform (no NVS backend)");
#endif
}
