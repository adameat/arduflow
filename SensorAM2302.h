#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t P>
class TSensorAM2302 : public TActor {
public:
    
protected:
    TPin<P> Pin;

    void InitiateRequest() {
        Pin = true;
        delay(250);
        Pin.SetMode(OUTPUT);
        Pin = false;
        delay(20);
        Pin = true;
        delayMicroseconds(40);
        Pin.SetMode(INPUT_PULLUP);
        delayMicroseconds(10);
        // read
    }

    static volatile uint8_t Data[5];
    static volatile uint8_t Pos;

    static void OnInterrupt() {

    }
};

}