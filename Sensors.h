#pragma once
namespace AW {

struct TSensorValue {
	StringBuf Name;
	double Value;
};

struct TSensorSource {
	StringBuf Name;
	TTime Updated;
};

template <size_t Count> struct TSensor : TSensorSource {
	TSensorValue Values[Count];
};

struct TEventSensorData : TBasicEvent<TEventSensorData> {
	constexpr static TEventID EventID = 6; // TODO
	const TSensorSource& Source;
	const TSensorValue& Value;

	TEventSensorData(const TSensorSource& source, const TSensorValue& value)
		: Source(source)
		, Value(value) {}
};

struct TEventSensorMessage : TBasicEvent<TEventSensorMessage> {
	constexpr static TEventID EventID = 7; // TODO
	const TSensorSource& Source;
	String Message;

	TEventSensorMessage(const TSensorSource& source, const String& message)
		: Source(source)
		, Message(message) {}
};

}

#include "SensorMemory.h"
#include "SensorEnergy.h"
#include "SensorVoltage.h"
#include "SensorBMP280.h"
#include "SensorBME280.h"
#include "SensorINA219.h"
#include "SensorAM2320.h"
#include "SensorCT.h"
#include "SensorCounter.h"
