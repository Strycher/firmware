#include "AirplaneMode.h"

#if HAS_SCREEN || defined(ARCH_ESP32)

#include "NodeDB.h"
#include "configuration.h"
#include "main.h"

AirplaneMode &AirplaneMode::instance()
{
    static AirplaneMode singleton;
    return singleton;
}

bool AirplaneMode::isActive() const
{
    return !config.network.wifi_enabled && !config.bluetooth.enabled && !config.lora.tx_enabled;
}

void AirplaneMode::toggle()
{
    bool turningOn = !isActive();
    LOG_WARN("AirplaneMode: %s", turningOn ? "entering" : "exiting");

    config.network.wifi_enabled = !turningOn;
    config.bluetooth.enabled = !turningOn;
    config.lora.tx_enabled = !turningOn;

    if (nodeDB)
        nodeDB->saveToDisk(SEGMENT_CONFIG);

    rebootAtMsec = millis() + 2000;
}

#endif
