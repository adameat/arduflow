#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Address = 0x5c, uint8_t PowerPin = -1, typename WireType = TWire>
class TSensorAM2320 : public TActor {
public:
	TActor* Owner;
	TTime Period = TTime::MilliSeconds(8000);
	bool SendValues = true;
	TSensor<2> Sensor;

	enum ESensor {
		Temperature,
		Humidity
	};

	TSensorAM2320(TActor* owner, const String& name = "am2320")
		: Owner(owner)
	{
		Sensor.Name = name;
		Sensor.Values[ESensor::Temperature].Name = "temperature";
		Sensor.Values[ESensor::Humidity].Name = "humidity";
	}

protected:
	static constexpr auto PowerOnDelay = TTime::MilliSeconds(1200);
	WireType Wire;
	bool Powered = false;
	TPin<PowerPin> Power;
	unsigned long Errors = 0;

	void OnEvent(TEventPtr event, const TActorContext& context) override {
		switch (event->EventID) {
		case TEventBootstrap::EventID:
			return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
		case TEventReceive::EventID:
			return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
		}
	}

	void PowerOn() {
		if (!Powered) {
			Powered = true;
			if (PowerPin != -1) {
				Power = Powered;
			}
		}
	}

	void PowerOff() {
		if (Powered) {
			Powered = false;
			if (PowerPin != -1) {
				Power = Powered;
			}
		}
	}

	void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
		for (auto tries = 0; tries < 2; ++tries) {
			PowerOn();
			delay(PowerOnDelay.MilliSeconds());
			Wire.BeginTransmission(Address);
			Wire.EndTransmission();
			Wire.BeginTransmission(Address);
			Wire.Write(0x03);
			Wire.Write(0x08);
			Wire.Write(0x02);
			if (Wire.EndTransmission()) {
				delayMicroseconds(1600);
				if (Wire.RequestFrom(Address, 0x06) == 0x06) {
					uint8_t code;
					uint8_t length;
					uint16_t model;
					uint16_t crc16;
					Wire.Read(code);
					Wire.Read(length);
					Wire.Read(model);
					Wire.Read(crc16);
					if (code == 0x03 && length == 0x02) {
						context.Send(this, this, new AW::TEventReceive(context.Now + Period));
						context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "AM2320 on " << String(Address, 16)));
						break;
					}
				}
			}
			PowerOff();
			delay(3000);
		}
	}

	static uint16_t CRC16(const uint8_t* ptr, uint8_t length) {
		uint16_t crc = 0xFFFF;
		uint8_t s = 0x00;

		while (length--) {
			crc ^= *ptr++;
			for (s = 0; s < 8; ++s) {
				if ((crc & 0x01) != 0) {
					crc >>= 1;
					crc ^= 0xA001;
				} else crc >>= 1;
			}
		}
		return crc;
	}

	template <typename T>
	static uint16_t CRC16(const T& data) {
		return CRC16(reinterpret_cast<const uint8_t*>(&data), sizeof(data));
	}

	static void bswap(uint16_t& data) {
		data = (data >> 8) | (data << 8);
	}

	struct TData {
		uint8_t Code;
		uint8_t Length;
		uint16_t Humidity;
		uint16_t Temperature;
	};

	void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
		if (!Powered) {
			PowerOn();
			event->NotBefore = context.Now + PowerOnDelay;
			context.Resend(this, event.Release());
			return;
		}
		auto timeout = Period;
		bool good = false;
		//Wire.BeginTransmission(Address);
//		delayMicroseconds(1600);
		//Wire.EndTransmission();
		for (auto tries = 0; tries < 1; ++tries) {
			//delayMicroseconds(30);
			Wire.BeginTransmission(Address);
			Wire.EndTransmission(false);
			//delayMicroseconds(800);
			Wire.BeginTransmission(Address);
			Wire.Write(0x03);
			Wire.Write(0x00);
			Wire.Write(0x04);
			if (Wire.EndTransmission()) {
				delayMicroseconds(3000);
				if (Wire.RequestFrom(Address, 0x08) == 0x08) {
					TData data;
					uint16_t crc16;
					Wire.Read(data);
					Wire.Read(crc16);
					bswap(crc16);
					if (data.Code != 0x03) {
						/*StringStream stream;
						stream << "AM2320 wrong Code (" << data.Code << ")";
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));
						stream.clear();
						stream << data.Code << " vs " << 3;
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));*/
						//timeout = TTime::MilliSeconds(2000);
					} else if (data.Length != 4) {
						/*StringStream stream;
						stream << "AM2320 wrong Length";
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));
						stream.clear();
						stream << data.Length << " vs " << 4;
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));*/
						//timeout = TTime::MilliSeconds(2000);
					} else if (CRC16(data) != crc16) {
						/*StringStream stream;
						stream << "AM2320 wrong CRC16";
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));
						stream.clear();
						stream << String(crc16, 16) << " vs " << String(CRC16(data), 16);
						context.Send(this, Owner, new AW::TEventSensorMessage(stream));*/
						//timeout = TTime::MilliSeconds(2000);
					} else {
						bswap(data.Temperature);
						bswap(data.Humidity);
						Sensor.Values[ESensor::Temperature].Value = (float)data.Temperature / 10;
						Sensor.Values[ESensor::Humidity].Value = (float)data.Humidity / 10;
						Sensor.Updated = context.Now;
						if (SendValues) {
							context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Temperature]));
							context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Humidity]));
						}
						good = true;
					}
				}
			}
			if (good) {
				break;
			}
		}
		if (!good) {
			PowerOff();
			timeout = TTime::MilliSeconds(Period - PowerOnDelay.MilliSeconds());
			context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "error " << ++Errors));
		}
		event->NotBefore = context.Now + timeout;
		context.Resend(this, event.Release());
	}
};

}	
/*

#include "AM2320.h"
#include <Wire.h>
// 
// AM2320 Temperature & Humidity Sensor library for Arduino
// Сделана Тимофеевым Е.Н.

unsigned int CRC16(byte *ptr, byte length) {
	unsigned int crc = 0xFFFF;
	uint8_t s = 0x00;

	while (length--) {
		crc ^= *ptr++;
		for (s = 0; s < 8; s++) {
			if ((crc & 0x01) != 0) {
				crc >>= 1;
				crc ^= 0xA001;
			} else crc >>= 1;
		}
	}
	return crc;
}

AM2320::AM2320() {
	Wire.begin();
}

int AM2320::Read() {
	byte buf[8];
	for (int s = 0; s < 8; s++) buf[s] = 0x00;

	Wire.beginTransmission(AM2320_address);
	Wire.endTransmission();
	// запрос 4 байт (температуры и влажности)
	Wire.beginTransmission(AM2320_address);
	Wire.write(0x03);// запрос
	Wire.write(0x00); // с 0-го адреса
	Wire.write(0x04); // 4 байта
	if (Wire.endTransmission(1) != 0) return 1;
	delayMicroseconds(1600); //>1.5ms
							 // считываем результаты запроса
	Wire.requestFrom(AM2320_address, 0x08);
	for (int i = 0; i < 0x08; i++) buf[i] = Wire.read();

	// CRC check
	unsigned int Rcrc = buf[7] << 8;
	Rcrc += buf[6];
	if (Rcrc == CRC16(buf, 6)) {
		unsigned int temperature = ((buf[4] & 0x7F) << 8) + buf[5];
		t = temperature / 10.0;
		t = ((buf[4] & 0x80) >> 7) == 1 ? t * (-1) : t;

		unsigned int humidity = (buf[2] << 8) + buf[3];
		h = humidity / 10.0;
		return 0;
	}
	return 2;
}
*/