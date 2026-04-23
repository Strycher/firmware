#pragma once

#include "configuration.h"

#if HAS_SCREEN

#include <Arduino.h>

// Menu-driven RF kill-switch. Disables LoRa TX, WiFi, and BLE by persisting
// config flags to false and rebooting. Exit restores all three. ESP32 NimBLE
// deinit is one-way so a reboot is required to restore BLE anyway; entering
// also reboots for symmetry and to avoid tearing down live connections.
class AirplaneMode
{
  public:
    static AirplaneMode &instance();

    // True when all three radio flags are false in the live config.
    bool isActive() const;

    // Toggle enter/exit. Persists config, schedules reboot.
    void toggle();
};

#endif
