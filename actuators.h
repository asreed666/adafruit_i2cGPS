#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "constants.h"


#define defLightThresh 50.0f
#define defTempThresh 20.0f
#define sixtyFourBitAligned 64

struct things_t {
    float lightLevelVal = defLightThresh;
    float lightThreshVal = defLightThresh;
    float temperatureVal= defTempThresh;
    float tempThreshVal = defTempThresh;
    bool redLedState = false;
    bool greenLedState = false;
    bool blueLedState = false;
    bool statusLedState = false;
    bool orangeLedState = false;
    bool lightState = false;
    bool lightStwitchState = false;
    bool heaterState = false;
    bool heaterSwitchState = false;
    bool localModeState = true;
    bool manualModeState = false;
    int rxCountVal = 0;
    int txCountVal = 0;
    float relHumidityVal = defHumidThresh;
//    char ipAddress[17] = "192.168.1.xxx";
    bool changed{};

} __attribute__((aligned(sixtyFourBitAligned))) ;

extern things_t thingData;

void actuatorThread();
#endif