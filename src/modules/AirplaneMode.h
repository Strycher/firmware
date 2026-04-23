#pragma once

#include "configuration.h"

#include <Arduino.h>

// RF kill-switch. Disables LoRa TX, WiFi, and BLE by persisting config flags
// and rebooting. Exit restores the pre-airplane state of each radio.
//
// State of record is an explicit NVS flag (not derived from live config), so
// external config mutations (e.g. phone app re-enabling a radio mid-flight)
// don't confuse the enter/exit transition. Today only the ESP32 backend
// persists state; on other platforms toggle() is a no-op.
class AirplaneMode
{
  public:
    static AirplaneMode &instance();

    // True when airplane mode is active per the persisted flag.
    bool isActive() const;

    // Toggle enter/exit. Persists config + state flag, schedules reboot.
    void toggle();
};
