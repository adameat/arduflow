#pragma once

#include "BluetoothHC05.h"
#include "BluetoothHC06.h"
#include "BluetoothZS040.h"

namespace AW {

template <typename BluetoothType>
class TBluetoothActor : public TActor {
public:
	TBluetoothActor(TActor* owner, TActor* serial)
		: Owner(owner)
		, Serial(serial)
		, Bluetooth(this, Serial)
	{}

	bool IsOK() const {
		return Bluetooth.IsOK();
	}

	bool IsConnected() const {
		return Bluetooth.IsConnected();
	}

protected:
	TActor* Owner;
	TActor* Serial;
	BluetoothType Bluetooth;

	void OnEvent(TEventPtr event, const TActorContext& context) override {
		switch (event->EventID) {
		case TEventBootstrap::EventID:
			return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
		case TEventSerialData::EventID:
			return OnSerialData(static_cast<TEventSerialData*>(event.Release()), context);
		case TEventReceive::EventID:
			return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
		}
	}

	void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
		Bluetooth.Init(context);
	}

	void OnSerialData(TUniquePtr<TEventSerialData> event, const TActorContext& context) {
		//::Serial.println("== ");
		if (event->Sender == Serial) {
			if (!Bluetooth.Receive(event, context)) {
				context.Send(this, Owner, event.Release());
			}
		} else {
			context.Send(this, Serial, event.Release());
		}
	}

	void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
		Bluetooth.Receive(event, context);
	}
};

}
