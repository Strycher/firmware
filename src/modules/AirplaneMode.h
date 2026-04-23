#pragma once

#include "configuration.h"

#if HAS_SCREEN || defined(ARCH_ESP32)

#include <Arduino.h>

/**
 * AirplaneMode — menu-driven RF kill-switch for flight-safe Meshtastic operation.
 *
 * Design rationale (supersedes the earlier button-gesture version):
 *   The previous design hooked into ButtonThread with an 8-12s hold gesture.
 *   That tore down WiFi/BLE/LoRa synchronously from the button-event context,
 *   which panicked the ESP32 when applied to a device with live TCP / BLE
 *   sessions. The menu-driven design instead:
 *     1. Persists the "airplane off" radio flags to NVS via nodeDB->saveToDisk()
 *     2. Schedules a clean reboot via rebootAtMsec
 *     3. Lets the normal boot path bring (or not bring) radios up based on the
 *        saved config.
 *   This is symmetric on enter and exit and avoids live-teardown crashes.
 *
 *   BLE-return on exit *requires* a reboot on ESP32 anyway — NimBLEDevice::deinit()
 *   is one-way — so the extra reboot on enter is not a meaningful cost.
 *
 * Persistence model:
 *   We save the "pre-airplane" radio state to the ESP32 Preferences NVS namespace
 *   so exit can restore faithfully. No new protobuf fields are introduced. The
 *   in-config airplane-mode indicator is simply "all three radio flags are false."
 */
class AirplaneMode
{
  public:
    static AirplaneMode &instance();

    // True when all three radio flags (lora.tx_enabled, network.wifi_enabled,
    // bluetooth.enabled) are currently false in the live config. No separate
    // state machine — the config state is the source of truth.
    bool isActive() const;

    // Menu entry point. If currently active, restores saved radio state and
    // reboots. If currently inactive, saves current radio state and disables
    // all three radios, then reboots. Caller is responsible for showing any
    // transient UI banner before calling.
    void toggle();

  private:
    AirplaneMode() = default;
    AirplaneMode(const AirplaneMode &) = delete;
    AirplaneMode &operator=(const AirplaneMode &) = delete;

    void enterActive();
    void exitActive();

    // NVS helpers — use the "meshtastic" Preferences namespace, matching main-esp32.cpp
    void saveRadioStateToNvs(bool wifi, bool bluetooth, bool loraTx);
    bool loadSavedRadioStateFromNvs(bool &outWifi, bool &outBluetooth, bool &outLoraTx);
};

#endif // HAS_SCREEN || ARCH_ESP32
