#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Pin, uint16_t MinDelayLow = 500, uint16_t MinDelayHigh = 500>
class TSensorCounter : public TActor {
public:
	TActor* Owner;
	TTime Period = TTime::MilliSeconds(1000);
	bool SendValues = true;
	TSensor<3> Sensor;

	enum ESensor {
		Counter,
		DelayLow,
		DelayHigh
	};
	
	TSensorCounter(TActor* owner, StringBuf name = "counter")
		: Owner(owner)
	{
		Sensor.Name = name;
		Sensor.Values[ESensor::Counter].Name = "counter";
		Sensor.Values[ESensor::DelayLow].Name = "low";
		Sensor.Values[ESensor::DelayHigh].Name = "high";

	}

protected:
	TPin<Pin, INPUT_PULLUP> PinValue;
	volatile bool LastValue;
	volatile uint32_t LastTime;
	volatile uint32_t Value;
	volatile uint32_t Low;
	volatile uint32_t High;
	
	void OnEvent(TEventPtr event, const TActorContext& context) override {
		switch (event->EventID) {
		case TEventBootstrap::EventID:
			return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
		case TEventReceive::EventID:
			return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
		}
	}

	void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
		This() = this;
		Value = 0;
		Low = 0;
		High = 0;
		LastValue = PinValue;
		LastTime = context.Now.MilliSeconds();
		attachInterrupt(digitalPinToInterrupt(Pin), StaticInterrupt, CHANGE);
		context.Send(this, this, new TEventReceive(context.Now + Period));
	}

	void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
		if (Value != Sensor.Values[ESensor::Counter].Value) {
			Sensor.Values[ESensor::Counter].Value = Value;
			Sensor.Values[ESensor::DelayLow].Value = Low;
			Sensor.Values[ESensor::DelayHigh].Value = High;
			Sensor.Updated = context.Now;
			if (SendValues) {
				context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::Counter]));
				context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::DelayLow]));
				context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::DelayHigh]));
			}
		}
		event->NotBefore = context.Now + Period;
		context.Resend(this, event.Release());
	}

	void Interrupt() {
		bool value = PinValue;
		if (value != LastValue) {
			TTime now = TTime::Now();
			TTime delay = now - TTime::MilliSeconds(LastTime);
			switch (value) {
			case false:
				if (delay < TTime::MilliSeconds(MinDelayLow)) {
					LastValue = value;
					return;
				}
				High = now;
				break;
			case true:
				if (delay < TTime::MilliSeconds(MinDelayHigh)) {
					LastValue = value;
					return;
				}
				Low = now;
				++Value;
				break;
			}
			LastTime = now.MilliSeconds();
			LastValue = value;
		}
	}

	static TSensorCounter*& This() { static TSensorCounter* _this; return _this; }

	static void StaticInterrupt() {
		This()->Interrupt();
	}
};

}
