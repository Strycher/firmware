#include "AirplaneMode.h"

#if HAS_SCREEN || defined(ARCH_ESP32)

#include "NodeDB.h"
#include "configuration.h"
#include "main.h"

#ifdef ARCH_ESP32
#include <Preferences.h>
#endif

// NVS key names — kept short to fit in NVS's 15-char key limit.
// Namespace is shared with main-esp32.cpp's "meshtastic" namespace.
static const char *kNvsNamespace = "meshtastic";
static const char *kKeySavedWifi = "apm_saved_wifi";   // 14 chars
static const char *kKeySavedBt = "apm_saved_bt";       // 12 chars
static const char *kKeySavedLoraTx = "apm_saved_lora"; // 14 chars

AirplaneMode &AirplaneMode::instance()
{
    static AirplaneMode singleton;
    return singleton;
}

bool AirplaneMode::isActive() const
{
    // Airplane mode = all three radio flags disabled in the live config.
    return (!config.network.wifi_enabled) && (!config.bluetooth.enabled) && (!config.lora.tx_enabled);
}

void AirplaneMode::toggle()
{
    if (isActive())
        exitActive();
    else
        enterActive();
}

void AirplaneMode::enterActive()
{
    LOG_WARN("AirplaneMode: entering — saving radio state and rebooting");

    // Stash the current (pre-airplane) radio state into NVS so exit can restore exactly.
    saveRadioStateToNvs(config.network.wifi_enabled, config.bluetooth.enabled, config.lora.tx_enabled);

    // Disable all three. No synchronous teardown here — the reboot handles it.
    config.network.wifi_enabled = false;
    config.bluetooth.enabled = false;
    config.lora.tx_enabled = false;

    // Persist config so post-reboot boot brings up nothing.
    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    // Schedule clean reboot. 2s gives the UI banner time to render and any
    // outstanding flush to complete. Matches the pattern used elsewhere (see
    // menuHandler::rebootMenu).
    rebootAtMsec = millis() + 2000;
}

void AirplaneMode::exitActive()
{
    LOG_WARN("AirplaneMode: exiting — restoring radio state and rebooting");

    bool savedWifi = true, savedBt = true, savedLoraTx = true;
    bool hasSaved = loadSavedRadioStateFromNvs(savedWifi, savedBt, savedLoraTx);

    if (!hasSaved) {
        // NVS had no saved state (e.g. factory reset while in airplane mode).
        // Safe default: turn everything on so the user can re-reach the node.
        LOG_WARN("AirplaneMode: no saved radio state in NVS; defaulting to all radios on");
        savedWifi = true;
        savedBt = true;
        savedLoraTx = true;
    }

    config.network.wifi_enabled = savedWifi;
    config.bluetooth.enabled = savedBt;
    config.lora.tx_enabled = savedLoraTx;

    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    rebootAtMsec = millis() + 2000;
}

// ---- NVS helpers -------------------------------------------------------------

void AirplaneMode::saveRadioStateToNvs(bool wifi, bool bluetooth, bool loraTx)
{
#ifdef ARCH_ESP32
    Preferences prefs;
    if (!prefs.begin(kNvsNamespace, false)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(%s) failed — saved radio state WILL NOT persist", kNvsNamespace);
        return;
    }
    prefs.putBool(kKeySavedWifi, wifi);
    prefs.putBool(kKeySavedBt, bluetooth);
    prefs.putBool(kKeySavedLoraTx, loraTx);
    prefs.end();
    LOG_INFO("AirplaneMode: saved radio state to NVS (wifi=%d, bt=%d, loraTx=%d)", wifi, bluetooth, loraTx);
#else
    (void)wifi;
    (void)bluetooth;
    (void)loraTx;
#endif
}

bool AirplaneMode::loadSavedRadioStateFromNvs(bool &outWifi, bool &outBluetooth, bool &outLoraTx)
{
#ifdef ARCH_ESP32
    Preferences prefs;
    if (!prefs.begin(kNvsNamespace, true)) {
        LOG_ERROR("AirplaneMode: Preferences.begin(%s, ro) failed", kNvsNamespace);
        return false;
    }
    // isKey() available on Preferences; use it to distinguish "never saved" from "saved false".
    bool haveAll = prefs.isKey(kKeySavedWifi) && prefs.isKey(kKeySavedBt) && prefs.isKey(kKeySavedLoraTx);
    if (haveAll) {
        outWifi = prefs.getBool(kKeySavedWifi, true);
        outBluetooth = prefs.getBool(kKeySavedBt, true);
        outLoraTx = prefs.getBool(kKeySavedLoraTx, true);
        LOG_INFO("AirplaneMode: loaded saved radio state from NVS (wifi=%d, bt=%d, loraTx=%d)", outWifi, outBluetooth, outLoraTx);
    }
    prefs.end();
    return haveAll;
#else
    (void)outWifi;
    (void)outBluetooth;
    (void)outLoraTx;
    return false;
#endif
}

#endif // HAS_SCREEN || ARCH_ESP32
