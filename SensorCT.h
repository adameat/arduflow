#pragma once

#include "ArduinoWorkflow.h"
#include <EmonLib.h>

namespace AW {

template <uint8_t Pin>
class TSensorCT : public TActor {
public:
	TActor* Owner;
	TTime Period = AW::TTime::MilliSeconds(3000);
	bool SendValues = true;
	TSensor<1> Sensor;

	enum ESensor {
		Current
	};

	TSensorCT(TActor* owner, double calibration = 28, StringBuf name = "ct")
		: Owner(owner)
	{
		EMon.current(Pin, calibration);
		Sensor.Name = name;
		Sensor.Values[ESensor::Current].Name = "current";
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
		context.Send(this, this, new AW::TEventReceive(context.Now + Period));
	}

	void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
		Sensor.Values[ESensor::Current].Value = EMon.calcIrms(1480);
		Sensor.Updated = context.Now;
		if (SendValues)
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Current]));
		event->NotBefore = context.Now + Period;
		context.Resend(this, event.Release());
	}
};

}