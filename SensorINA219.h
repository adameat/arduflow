#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <uint8_t Address = 0x40, typename Env = TDefaultEnvironment>
class TSensorINA219 : public TActor {
    constexpr static bool UseChipCalculations = false;

    struct ERegisters {
        static constexpr uint8_t INA219_REG_CONFIG = 0x00;
        static constexpr uint8_t INA219_REG_SHUNTVOLTAGE = 0x01;
        static constexpr uint8_t INA219_REG_BUSVOLTAGE = 0x02;
        static constexpr uint8_t INA219_REG_POWER = 0x03;
        static constexpr uint8_t INA219_REG_CURRENT = 0x04;
        static constexpr uint8_t INA219_REG_CALIBRATION = 0x05;
    };

    enum EFlags : uint16_t {
        /*---------------------------------------------------------------------*/
        INA219_CONFIG_RESET = 0x8000,  // Reset Bit

        INA219_CONFIG_BVOLTAGERANGE_MASK = 0x2000,  // Bus Voltage Range Mask
        INA219_CONFIG_BVOLTAGERANGE_16V = 0x0000,  // 0-16V Range
        INA219_CONFIG_BVOLTAGERANGE_32V = 0x2000,  // 0-32V Range

        INA219_CONFIG_GAIN_MASK = 0x1800,  // Gain Mask
        INA219_CONFIG_GAIN_1_40MV = 0x0000,  // Gain 1, 40mV Range
        INA219_CONFIG_GAIN_2_80MV = 0x0800,  // Gain 2, 80mV Range
        INA219_CONFIG_GAIN_4_160MV = 0x1000,  // Gain 4, 160mV Range
        INA219_CONFIG_GAIN_8_320MV = 0x1800,  // Gain 8, 320mV Range

        INA219_CONFIG_BADCRES_MASK = 0x0780,  // Bus ADC Resolution Mask
        INA219_CONFIG_BADCRES_9BIT = 0x0080,  // 9-bit bus res = 0..511
        INA219_CONFIG_BADCRES_10BIT = 0x0100,  // 10-bit bus res = 0..1023
        INA219_CONFIG_BADCRES_11BIT = 0x0200,  // 11-bit bus res = 0..2047
        INA219_CONFIG_BADCRES_12BIT = 0x0400,  // 12-bit bus res = 0..4097

        INA219_CONFIG_SADCRES_MASK = 0x0078,  // Shunt ADC Resolution and Averaging Mask
        INA219_CONFIG_SADCRES_9BIT_1S_84US = 0x0000,  // 1 x 9-bit shunt sample
        INA219_CONFIG_SADCRES_10BIT_1S_148US = 0x0008,  // 1 x 10-bit shunt sample
        INA219_CONFIG_SADCRES_11BIT_1S_276US = 0x0010,  // 1 x 11-bit shunt sample
        INA219_CONFIG_SADCRES_12BIT_1S_532US = 0x0018,  // 1 x 12-bit shunt sample
        INA219_CONFIG_SADCRES_12BIT_2S_1060US = 0x0048,	 // 2 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_4S_2130US = 0x0050,  // 4 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_8S_4260US = 0x0058,  // 8 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_16S_8510US = 0x0060,  // 16 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_32S_17MS = 0x0068,  // 32 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_64S_34MS = 0x0070,  // 64 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_128S_69MS = 0x0078,  // 128 x 12-bit shunt samples averaged together

        INA219_CONFIG_MODE_MASK = 0x0007,  // Operating Mode Mask
        INA219_CONFIG_MODE_POWERDOWN = 0x0000,
        INA219_CONFIG_MODE_SVOLT_TRIGGERED = 0x0001,
        INA219_CONFIG_MODE_BVOLT_TRIGGERED = 0x0002,
        INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED = 0x0003,
        INA219_CONFIG_MODE_ADCOFF = 0x0004,
        INA219_CONFIG_MODE_SVOLT_CONTINUOUS = 0x0005,
        INA219_CONFIG_MODE_BVOLT_CONTINUOUS = 0x0006,
        INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS = 0x0007,
        /*=========================================================================*/

        
    };

    struct TConfigRegister {
        uint16_t Mode : 3;
        uint16_t SADC : 4;
        uint16_t BADC : 4;
        uint16_t PG : 2;
        uint16_t BRNG : 1;
        uint16_t RSRVD : 1;
        uint16_t RESET : 1;
    };

    struct TBusVoltageRegister {
        int16_t OVF : 1;
        int16_t CNVR : 1;
        int16_t RSRVD : 1;
        int16_t Value : 13;
    };
    
public:
    TActor* Owner;
    TSensor<3> Sensor;

    enum ESensor {
        Voltage,
        Current,
        Power,
    };

    TSensorINA219(TActor* owner, StringBuf name = "ina219")
        : Owner(owner)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Voltage].Name = "voltage";
        Sensor.Values[ESensor::Current].Name = "current";
        Sensor.Values[ESensor::Power].Name = "power";
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
        static constexpr uint16_t ConfigValue =
            EFlags::INA219_CONFIG_BVOLTAGERANGE_16V |
            EFlags::INA219_CONFIG_GAIN_1_40MV |
            EFlags::INA219_CONFIG_BADCRES_12BIT |
            EFlags::INA219_CONFIG_SADCRES_12BIT_128S_69MS |
            EFlags::INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

        if (Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CONFIG, ConfigValue)) {
            if (UseChipCalculations) {
                static constexpr uint16_t CalibrationValue = 32768;
                Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CALIBRATION, CalibrationValue);
            }
            context.Send(this, this, new AW::TEventReceive(context.Now + Env::SensorsPeriod));
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "INA219 on " << String(Address, 16)));
            }
        }
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());

        uint16_t config_value = 0; // INA219_REG_CONFIG
        TConfigRegister& config(*reinterpret_cast<TConfigRegister*>(&config_value));
        
        uint16_t shunt_voltage = 0;
        uint16_t bus_voltage_value = 0;
        TBusVoltageRegister& bus_voltage(*reinterpret_cast<TBusVoltageRegister*>(&bus_voltage_value));
        

        Env::Wire::ReadValue(Address, ERegisters::INA219_REG_CONFIG, config_value);
        

        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "config " << String(config_value, 16)));
        }

        static constexpr float RSHUNT = 0.1; // ohms
        static constexpr float shuntLSB = 0.010; // mV
        static constexpr float voltageLSB = 0.004; // V
        
        Env::Wire::ReadValue(Address, ERegisters::INA219_REG_SHUNTVOLTAGE, shunt_voltage);
        Env::Wire::ReadValue(Address, ERegisters::INA219_REG_BUSVOLTAGE, bus_voltage_value);

        float busValue = bus_voltage.Value * voltageLSB;
        float shuntValue = ((int32_t)(int16_t)shunt_voltage << config.PG) * shuntLSB;

        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "bus " << String(bus_voltage_value, 16) << " (" << busValue << ")"));
            context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "shunt " << String(shunt_voltage, 16) << " (" << shuntValue << ")"));
        }

        if (shunt_voltage == 0xffff || bus_voltage.OVF) { // not connected or overflowed
            Sensor.Values[ESensor::Voltage].Value = 0;
            Sensor.Values[ESensor::Current].Value = 0;
            Sensor.Values[ESensor::Power].Value = 0;
            if (Env::SensorsCalibration) {
                if (bus_voltage.OVF) {
                    if (config.PG < 3) {
                        ++config.PG;
                        Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CONFIG, config_value);
                        return;
                    }
                }
            }
        } else {
            Sensor.Values[ESensor::Voltage].Value = busValue;

            if (Env::SensorsCalibration) {
                float shuntValue = ((int32_t)(int16_t)shunt_voltage << config.PG) * shuntLSB;
                if (config.PG > 0) {
                    float maxShuntValue = ((int32_t)(int16_t)0x7fff << config.PG) * shuntLSB;
                    if (shuntValue < 0.50 * maxShuntValue) {
                        --config.PG;
                        Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CONFIG, config_value);
                    }
                }
            }

            if (UseChipCalculations) {
                uint16_t calibration_value = 0;
                int16_t current_value = 0;
                int16_t power_value = 0;

                Env::Wire::ReadValue(Address, ERegisters::INA219_REG_CALIBRATION, calibration_value);
                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "calibration " << String(calibration_value, 16)));
                }

                //float currentLSB = 0.04096 / (calibration_value * RSHUNT); // A
                float currentLSB = 40.96 / (calibration_value * RSHUNT); // mA
                float powerLSB = currentLSB * 20; // mW

                Env::Wire::ReadValue(Address, ERegisters::INA219_REG_CURRENT, current_value);
                Env::Wire::ReadValue(Address, ERegisters::INA219_REG_POWER, power_value);

                float currentValue = current_value * currentLSB;
                Sensor.Values[ESensor::Current].Value = currentValue;
                float powerValue = power_value * powerLSB;
                if (currentValue < 0) {
                    powerValue = -powerValue;
                }
                Sensor.Values[ESensor::Power].Value = powerValue;

                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "current " << String(current_value, 16) << " (" << Sensor.Values[ESensor::Current].Value << ")"));
                    context.Send(this, Owner, new AW::TEventSensorMessage(Sensor, StringStream() << "power " << String(power_value, 16) << " (" << Sensor.Values[ESensor::Power].Value << ")"));
                }

                if (Env::SensorsCalibration) {
                    float maxCurrentValue = 0x7fff * currentLSB;
                    static constexpr uint16_t calibration_value_step = 1024;
                    if (abs(currentValue) < 0.40 * maxCurrentValue) {
                        if (calibration_value <= (0xffff - calibration_value_step * 2)) {
                            calibration_value += calibration_value_step;
                            Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CALIBRATION, calibration_value);
                        }
                    } else if (abs(currentValue) > 0.90 * maxCurrentValue) {
                        if (calibration_value >= calibration_value_step * 2) {
                            calibration_value -= calibration_value_step;
                            Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CALIBRATION, calibration_value);
                        }
                    }
                }
            } else {
                float currentValue = shuntValue / RSHUNT;
                Sensor.Values[ESensor::Current].Value = currentValue;
                Sensor.Values[ESensor::Power].Value = busValue * currentValue;
            }
        }

        /*Wire.BeginTransmission(Address);
        Wire.Write(ERegisters::INA219_REG_CONFIG);
        Wire.Write(EFlags::INA219_CONFIG_MODE_POWERDOWN);
        Wire.EndTransmission();*/

        Sensor.Updated = context.Now;

        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Power]));
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Voltage]));
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Current]));
        }
    }
};

}