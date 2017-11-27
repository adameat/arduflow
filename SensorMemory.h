#pragma once

#include "ArduinoWorkflow.h"

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

namespace AW {

class TSensorMemory : public TActor {
public:
    TActor* Owner;
    TTime Period = AW::TTime::MilliSeconds(3000);
    bool SendValues = true;
    TSensor<1> Sensor;

    enum ESensor {
        Free
    };

    TSensorMemory(TActor* owner)
        : Owner(owner)
    {
        Sensor.Name = "memory";
        Sensor.Values[ESensor::Free].Name = "free";
    }

protected:
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        context.Send(this, this, new AW::TEventReceive(context.Now + Period));
    }

    static uint16_t GetFreeMemory() {
        uint16_t freeMemory;
        if ((int)__brkval == 0)
            return ((uint16_t)&freeMemory) - ((uint16_t)&__bss_end);
        else
            return ((uint16_t)&freeMemory) - ((uint16_t)__brkval);
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        Sensor.Values[ESensor::Free].Value = GetFreeMemory();
        Sensor.Updated = context.Now;
        if (SendValues)
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Free]));
        event->NotBefore = context.Now + Period;
        context.Resend(this, event.Release());
    }
};

}