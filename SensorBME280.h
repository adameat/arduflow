#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Address = 0x77, typename Env = TDefaultEnvironment>
class TSensorBME280 : public TActor {
	static constexpr uint8_t ChipID = 0x60;

	enum ERegisters : uint8_t {
		BME280_REGISTER_DIG_T1 = 0x88,
		BME280_REGISTER_DIG_T2 = 0x8A,
		BME280_REGISTER_DIG_T3 = 0x8C,

		BME280_REGISTER_DIG_P1 = 0x8E,
		BME280_REGISTER_DIG_P2 = 0x90,
		BME280_REGISTER_DIG_P3 = 0x92,
		BME280_REGISTER_DIG_P4 = 0x94,
		BME280_REGISTER_DIG_P5 = 0x96,
		BME280_REGISTER_DIG_P6 = 0x98,
		BME280_REGISTER_DIG_P7 = 0x9A,
		BME280_REGISTER_DIG_P8 = 0x9C,
		BME280_REGISTER_DIG_P9 = 0x9E,

		BME280_REGISTER_DIG_H1 = 0xA1,
		BME280_REGISTER_DIG_H2 = 0xE1,
		BME280_REGISTER_DIG_H3 = 0xE3,
		BME280_REGISTER_DIG_H4 = 0xE4,
		BME280_REGISTER_DIG_H5 = 0xE5,
		BME280_REGISTER_DIG_H6 = 0xE7,

		BME280_REGISTER_CHIPID = 0xD0,
		BME280_REGISTER_VERSION = 0xD1,
		BME280_REGISTER_SOFTRESET = 0xE0,

		BME280_REGISTER_CAL26 = 0xE1,  // R calibration stored in 0xE1-0xF0

		BME280_REGISTER_CONTROLHUMID = 0xF2,
		BME280_REGISTER_STATUS = 0XF3,
		BME280_REGISTER_CONTROL = 0xF4,
		BME280_REGISTER_CONFIG = 0xF5,
		BME280_REGISTER_PRESSUREDATA = 0xF7,
		BME280_REGISTER_TEMPDATA = 0xFA,
		BME280_REGISTER_HUMIDDATA = 0xFD,
	};

	struct BME280CalibData {
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

	enum sensor_sampling {
		SAMPLING_NONE = 0b000,
		SAMPLING_X1 = 0b001,
		SAMPLING_X2 = 0b010,
		SAMPLING_X4 = 0b011,
		SAMPLING_X8 = 0b100,
		SAMPLING_X16 = 0b101,
	};

	enum sensor_mode {
		MODE_SLEEP = 0b00,
		MODE_FORCED = 0b01,
		MODE_NORMAL = 0b11,
	};

	enum sensor_filter {
		FILTER_OFF = 0b000,
		FILTER_X2 = 0b001,
		FILTER_X4 = 0b010,
		FILTER_X8 = 0b011,
		FILTER_X16 = 0b100,
	};

	// standby durations in ms 
	enum standby_duration {
		STANDBY_MS_0_5 = 0b000,
		STANDBY_MS_10 = 0b110,
		STANDBY_MS_20 = 0b111,
		STANDBY_MS_62_5 = 0b001,
		STANDBY_MS_125 = 0b010,
		STANDBY_MS_250 = 0b011,
		STANDBY_MS_500 = 0b100,
		STANDBY_MS_1000 = 0b101,
	};

	// The config register
	struct config {
		// inactive duration (standby time) in normal mode
		// 000 = 0.5 ms
		// 001 = 62.5 ms
		// 010 = 125 ms
		// 011 = 250 ms
		// 100 = 500 ms
		// 101 = 1000 ms
		// 110 = 10 ms
		// 111 = 20 ms
		unsigned int t_sb : 3;

		// filter settings
		// 000 = filter off
		// 001 = 2x filter
		// 010 = 4x filter
		// 011 = 8x filter
		// 100 and above = 16x filter
		unsigned int filter : 3;

		// unused - don't set
		unsigned int none : 1;
		unsigned int spi3w_en : 1;

		unsigned int get() {
			return (t_sb << 5) | (filter << 3) | spi3w_en;
		}
	};
	config _configReg;

	// The ctrl_meas register
	struct ctrl_meas {
		// temperature oversampling
		// 000 = skipped
		// 001 = x1
		// 010 = x2
		// 011 = x4
		// 100 = x8
		// 101 and above = x16
		unsigned int osrs_t : 3;

		// pressure oversampling
		// 000 = skipped
		// 001 = x1
		// 010 = x2
		// 011 = x4
		// 100 = x8
		// 101 and above = x16
		unsigned int osrs_p : 3;

		// device mode
		// 00       = sleep
		// 01 or 10 = forced
		// 11       = normal
		unsigned int mode : 2;

		unsigned int get() {
			return (osrs_t << 5) | (osrs_p << 3) | mode;
		}
	};
	ctrl_meas _measReg;
	
	// The ctrl_hum register
	struct ctrl_hum {
		// unused - don't set
		unsigned int none : 5;

		// pressure oversampling
		// 000 = skipped
		// 001 = x1
		// 010 = x2
		// 011 = x4
		// 100 = x8
		// 101 and above = x16
		unsigned int osrs_h : 3;

		unsigned int get() {
			return (osrs_h);
		}
	};
	ctrl_hum _humReg;

public:
	TActor* Owner;
	TSensor<3> Sensor;

	enum ESensor {
		Temperature,
		Pressure,
		Humidity,
	};

	TSensorBME280(TActor* owner, StringBuf name = "bme280")
		: Owner(owner)
	{
		Sensor.Name = name;
		Sensor.Values[ESensor::Temperature].Name = "temperature";
		Sensor.Values[ESensor::Pressure].Name = "pressure";
		Sensor.Values[ESensor::Humidity].Name = "humidity";
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
		uint8_t chipID = Read8(ERegisters::BME280_REGISTER_CHIPID);
		if (chipID == ChipID) {
			/*
			// reset the device using soft-reset
			// this makes sure the IIR is off, etc.
			write8(BME280_REGISTER_SOFTRESET, 0xB6);

			// wait for chip to wake up.
			delay(300);

			// if chip is still reading calibration, delay
			while (isReadingCalibration())
				delay(100);
			*/

			sensor_mode mode = MODE_NORMAL;
			sensor_sampling tempSampling = SAMPLING_X16;
			sensor_sampling pressSampling = SAMPLING_X16;
			sensor_sampling humSampling = SAMPLING_X16;
			sensor_filter filter = FILTER_OFF;
			standby_duration duration = STANDBY_MS_0_5;

			_measReg.mode = mode;
			_measReg.osrs_t = tempSampling;
			_measReg.osrs_p = pressSampling;
			_humReg.osrs_h = humSampling;
			_configReg.filter = filter;
			_configReg.t_sb = duration;

			// you must make sure to also set REGISTER_CONTROL after setting the
			// CONTROLHUMID register, otherwise the values won't be applied (see DS 5.4.3)
			Write8(BME280_REGISTER_CONTROLHUMID, _humReg.get());
			Write8(BME280_REGISTER_CONFIG, _configReg.get());
			Write8(BME280_REGISTER_CONTROL, _measReg.get());

			context.Send(this, this, new AW::TEventReceive(context.Now + Env::SensorsPeriod));
			if (Env::Diagnostics) {
				context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "BME280 on " << String(Address, 16)));
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

	static void Write8(uint8_t reg, uint8_t val) {
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::Write(val);
		Env::Wire::EndTransmission();
	}

	static uint16_t Read16(uint8_t reg) {
		uint16_t result;
		Env::Wire::BeginTransmission(Address);
		Env::Wire::Write(reg);
		Env::Wire::EndTransmission();
		Env::Wire::RequestFrom(Address, 2);
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

	void ReadCoefficients(BME280CalibData& data) {
		data.dig_T1 = Read16LE(BME280_REGISTER_DIG_T1);
		data.dig_T2 = ReadS16LE(BME280_REGISTER_DIG_T2);
		data.dig_T3 = ReadS16LE(BME280_REGISTER_DIG_T3);

		data.dig_P1 = Read16LE(BME280_REGISTER_DIG_P1);
		data.dig_P2 = ReadS16LE(BME280_REGISTER_DIG_P2);
		data.dig_P3 = ReadS16LE(BME280_REGISTER_DIG_P3);
		data.dig_P4 = ReadS16LE(BME280_REGISTER_DIG_P4);
		data.dig_P5 = ReadS16LE(BME280_REGISTER_DIG_P5);
		data.dig_P6 = ReadS16LE(BME280_REGISTER_DIG_P6);
		data.dig_P7 = ReadS16LE(BME280_REGISTER_DIG_P7);
		data.dig_P8 = ReadS16LE(BME280_REGISTER_DIG_P8);
		data.dig_P9 = ReadS16LE(BME280_REGISTER_DIG_P9);

		data.dig_H1 = Read8(BME280_REGISTER_DIG_H1);
		data.dig_H2 = ReadS16LE(BME280_REGISTER_DIG_H2);
		data.dig_H3 = Read8(BME280_REGISTER_DIG_H3);
		data.dig_H4 = (Read8(BME280_REGISTER_DIG_H4) << 4) | (Read8(BME280_REGISTER_DIG_H4 + 1) & 0xF);
		data.dig_H5 = (Read8(BME280_REGISTER_DIG_H5 + 1) << 4) | (Read8(BME280_REGISTER_DIG_H5) >> 4);
		data.dig_H6 = (int8_t)Read8(BME280_REGISTER_DIG_H6);

		/*Env::Wire::ReadValueLE(BME280_REGISTER_DIG_T1, data.dig_T1);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_T2, data.dig_T2);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_T3, data.dig_T3);

		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P1, data.dig_P1);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P2, data.dig_P2);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P3, data.dig_P3);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P4, data.dig_P4);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P5, data.dig_P5);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P6, data.dig_P6);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P7, data.dig_P7);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P8, data.dig_P8);
		Env::Wire::ReadValueLE(BME280_REGISTER_DIG_P9, data.dig_P9);

		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H1, data.dig_H1);
		Env::Wire::ReadValueLE(Address, BME280_REGISTER_DIG_H2, data.dig_H2);
		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H3, data.dig_H3);

		uint8_t h[3];
		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H4, h[0]);
		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H5, h[1]);
		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H5 + 1, h[2]);
		data.dig_H4 = (h[0] << 4) | (h[1] & 0xF);
		data.dig_H5 = (h[2] << 4) | (h[1] >> 4);

		Env::Wire::ReadValue(Address, BME280_REGISTER_DIG_H6, data.dig_H6);*/
	}

	void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
		BME280CalibData calib;
		ReadCoefficients(calib);
		int32_t t_fine;
		{
			int32_t var1, var2;

			int32_t adc_T = Read24(ERegisters::BME280_REGISTER_TEMPDATA);
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

			int32_t adc_P = Read24(ERegisters::BME280_REGISTER_PRESSUREDATA);
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
		{
			int32_t adc_H = Read16(ERegisters::BME280_REGISTER_HUMIDDATA);
			int32_t v_x1_u32r;

			v_x1_u32r = (t_fine - ((int32_t)76800));

			v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) -
				(((int32_t)calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
				(((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) *
				(((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
					((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));

			v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
				((int32_t)calib.dig_H1)) >> 4));

			v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
			v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
			float h = (v_x1_u32r >> 12);
			Sensor.Values[ESensor::Humidity].Value = h / 1024.0;
		}
		Sensor.Updated = context.Now;
		if (Env::SensorsSendValues) {
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Temperature]));
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Pressure]));
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Humidity]));
		}
		event->NotBefore = context.Now + Env::SensorsPeriod;
		context.Resend(this, event.Release());
	}
};

}