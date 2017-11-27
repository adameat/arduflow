#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Address = 0x77, typename Env = TDefaultEnvironment>
class TSensorBMP280 : public TActor {
	static constexpr uint8_t ChipID = 0x58;

	struct ERegisters {
		static constexpr uint8_t BMP280_REGISTER_DIG_T1 = 0x88;
		static constexpr uint8_t BMP280_REGISTER_DIG_T2 = 0x8A;
		static constexpr uint8_t BMP280_REGISTER_DIG_T3 = 0x8C;

		static constexpr uint8_t BMP280_REGISTER_DIG_P1 = 0x8E;
		static constexpr uint8_t BMP280_REGISTER_DIG_P2 = 0x90;
		static constexpr uint8_t BMP280_REGISTER_DIG_P3 = 0x92;
		static constexpr uint8_t BMP280_REGISTER_DIG_P4 = 0x94;
		static constexpr uint8_t BMP280_REGISTER_DIG_P5 = 0x96;
		static constexpr uint8_t BMP280_REGISTER_DIG_P6 = 0x98;
		static constexpr uint8_t BMP280_REGISTER_DIG_P7 = 0x9A;
		static constexpr uint8_t BMP280_REGISTER_DIG_P8 = 0x9C;
		static constexpr uint8_t BMP280_REGISTER_DIG_P9 = 0x9E;

		static constexpr uint8_t BMP280_REGISTER_CHIPID = 0xD0;
		static constexpr uint8_t BMP280_REGISTER_VERSION = 0xD1;
		static constexpr uint8_t BMP280_REGISTER_SOFTRESET = 0xE0;

		static constexpr uint8_t BMP280_REGISTER_CAL26 = 0xE1;  // R calibration stored in 0xE1-0xF0

		static constexpr uint8_t BMP280_REGISTER_CONTROL = 0xF4;
		static constexpr uint8_t BMP280_REGISTER_CONFIG = 0xF5;
		static constexpr uint8_t BMP280_REGISTER_PRESSUREDATA = 0xF7;
		static constexpr uint8_t BMP280_REGISTER_TEMPDATA = 0xFA;
	};

	struct EFlags {
		static constexpr uint8_t BMP280_RESET = 0x3F;
	};

	struct BMP280CalibData {
		uint16_t dig_T1;
		int16_t  dig_T2;
		int16_t  dig_T3;

		uint16_t dig_P1;
		int16_t  dig_P2;
		int16_t  dig_P3;
		int16_t  dig_P4;
		int16_t  dig_P5;
		int16_t  dig_P6;
		int16_t  dig_P7;
		int16_t  dig_P8;
		int16_t  dig_P9;

		uint8_t  dig_H1;
		int16_t  dig_H2;
		uint8_t  dig_H3;
		int16_t  dig_H4;
		int16_t  dig_H5;
		int8_t   dig_H6;
	};

public:
	TActor* Owner;
	TSensor<2> Sensor;

	enum ESensor {
		Temperature,
		Pressure
	};

	TSensorBMP280(TActor* owner, StringBuf name = "bmp280")
		: Owner(owner)
	{
		Sensor.Name = name;
		Sensor.Values[ESensor::Temperature].Name = "temperature";
		Sensor.Values[ESensor::Pressure].Name = "pressure";
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
		uint8_t chipID = Read8(ERegisters::BMP280_REGISTER_CHIPID);
		if (chipID == ChipID) {
			Env::Wire::WriteValue(Address, ERegisters::BMP280_REGISTER_CONTROL, EFlags::BMP280_RESET);
			context.Send(this, this, new AW::TEventReceive(context.Now + Env::SensorsPeriod));
			if (Env::Diagnostics) {
				context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "BMP280 on " << String(Address, 16)));
			}
		}

		/*Wire.BeginTransmission(Address);
		Wire.Write(0x3F);
		Wire.EndTransmission();*/

		//Wire << ERegisters::BMP280_REGISTER_CHIPID;
		//Wire >> chipID;
		
		/*Wire.BeginTransmission(Address);
		Wire.Write(ERegisters::BMP280_REGISTER_CHIPID);
		Wire.EndTransmission();
		Wire.RequestFrom(Address, 1);
		Wire.Read(chipID);*/
	}

	static uint8_t Read8(uint8_t reg) {
		uint8_t result;
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::EndTransmission();
		Env::Wire::RequestFrom(Address, 1);
		Env::Wire::Read(result);
		return result;
	}

	static uint16_t Read16LE(uint8_t reg) {
		uint16_t result;
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::EndTransmission();
		Env::Wire::RequestFrom(Address, 2);
		Env::Wire::ReadLE(result);
		return result;
	}

	static int16_t ReadS16LE(uint8_t reg) {
		int16_t result;
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::EndTransmission();
		Env::Wire::RequestFrom(Address, 2);
		Env::Wire::ReadLE(result);
		return result;
	}

	static uint32_t Read24(uint8_t reg) {
		union {
			uint8_t result_b[4];
			uint32_t result;
		} data;
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::EndTransmission();
		Env::Wire::RequestFrom(Address, 3);
		data.result_b[3] = 0;
		Env::Wire::Read(data.result_b[2]);
		Env::Wire::Read(data.result_b[1]);
		Env::Wire::Read(data.result_b[0]);
		return data.result;
	}

	void ReadCoefficients(BMP280CalibData& data) {
		data.dig_T1 = Read16LE(ERegisters::BMP280_REGISTER_DIG_T1);
		data.dig_T2 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_T2);
		data.dig_T3 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_T3);

		data.dig_P1 = Read16LE(ERegisters::BMP280_REGISTER_DIG_P1);
		data.dig_P2 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P2);
		data.dig_P3 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P3);
		data.dig_P4 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P4);
		data.dig_P5 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P5);
		data.dig_P6 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P6);
		data.dig_P7 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P7);
		data.dig_P8 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P8);
		data.dig_P9 = ReadS16LE(ERegisters::BMP280_REGISTER_DIG_P9);
	}

	void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
		BMP280CalibData calib;
		ReadCoefficients(calib);
		int32_t t_fine;
		{
			int32_t var1, var2;

			int32_t adc_T = Read24(ERegisters::BMP280_REGISTER_TEMPDATA);
			adc_T >>= 4;

			var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) *
				((int32_t)calib.dig_T2)) >> 11;

			var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
				((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
				((int32_t)calib.dig_T3)) >> 14;

			t_fine = var1 + var2;

			float T = (t_fine * 5 + 128) >> 8;
			Sensor.Values[ESensor::Temperature].Value = T / 100;
		}
		{
			int64_t var1, var2, p;

			int32_t adc_P = Read24(ERegisters::BMP280_REGISTER_PRESSUREDATA);
			adc_P >>= 4;

			var1 = ((int64_t)t_fine) - 128000;
			var2 = var1 * var1 * (int64_t)calib.dig_P6;
			var2 = var2 + ((var1*(int64_t)calib.dig_P5) << 17);
			var2 = var2 + (((int64_t)calib.dig_P4) << 35);
			var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
				((var1 * (int64_t)calib.dig_P2) << 12);
			var1 = (((((int64_t)1) << 47) + var1))*((int64_t)calib.dig_P1) >> 33;

			if (var1 != 0) {
				p = 1048576 - adc_P;
				p = (((p << 31) - var2) * 3125) / var1;
				var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
				var2 = (((int64_t)calib.dig_P8) * p) >> 19;

				p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
				float P = (float)p / 256;
				Sensor.Values[ESensor::Pressure].Value = P / 133.32239; // to mmHg
			}
		}
		Sensor.Updated = context.Now;
		if (Env::SensorsSendValues) {
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Temperature]));
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Pressure]));
		}
		event->NotBefore = context.Now + Env::SensorsPeriod;
		context.Resend(this, event.Release());
	}
};

}