#pragma once

#include "ArduinoWorkflow.h"
#include <EmonLib.h>

namespace AW {

class TSensorEnergy : public TActor {
public:
    TActor* Owner;
    TTime Period = AW::TTime::MilliSeconds(10000);
    bool SendValues = true;

    TSensorEnergy(TActor* owner, uint8_t vPin, double vCal, double phaseCal, uint8_t iPin, double iCal)
        : Owner(owner)
    {
        EMon.voltage(vPin, vCal, phaseCal);
        EMon.current(iPin, iCal);
    }

protected:
    EnergyMonitor EMon;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        EMon.calcVI(100, 1000);
        context.Send(this, this, new AW::TEventReceive(context.Now + Period));
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        EMon.calcVI(100, 1000);
        //context.Send(this, Owner, new AW::TEventSensorData("energy.power", EMon.apparentPower));
        //context.Send(this, Owner, new AW::TEventSensorData("energy.voltage", EMon.Vrms));
        //context.Send(this, Owner, new AW::TEventSensorData("energy.current", EMon.Irms));
        event->NotBefore = context.Now + Period;
        context.Resend(this, event.Release());
    }
};

}