#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Pin, int Multiplier = 1, int Divider = 1>
class TSensorVoltage : public TActor {
public:
    TActor* Owner;
    TTime Period = AW::TTime::MilliSeconds(3000);
    bool SendValues = true;
    TSensor<1> Sensor;

    enum ESensor {
        Voltage
    };
    
    TSensorVoltage(TActor* owner, StringBuf name = "some")
        : Owner(owner)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Voltage].Name = "voltage";
    }

protected:
    TPin<Pin, INPUT> PinValue;

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

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        float value = (float)PinValue.GetAveragedValue() * Multiplier / Divider;
        Sensor.Values[ESensor::Voltage].Value = value;
        Sensor.Updated = context.Now;
        if (SendValues)
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Voltage]));
        event->NotBefore = context.Now + Period;
        context.Resend(this, event.Release());
    }
};

}