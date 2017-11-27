#pragma once

/*
template <typename DerivedType, typename _SerialType = HardwareSerial>
class BluetoothHC06 : public SerialActivity<BluetoothHC06<DerivedType, _SerialType>, _SerialType> {
public:
    bool OK;

    BluetoothHC06()
        : OK(false)
    {}

    void OnCommand(const StringBuf& buf) {
        static_cast<DerivedType*>(this)->OnCommand(buf);
    }

    template <typename... Types>
    String BluetoothCommand(Types... args);

    template <typename... Types>
    String BluetoothCommand(const String& arg, Types... args) {
        Port.write(arg.c_str());
        return BluetoothCommand(args...);
    }

    template <typename... Types>
    String BluetoothCommand(const StringSumHelper& arg, Types... args) {
        Port.write(arg.c_str());
        return BluetoothCommand(args...);
    }

    template <typename Type, typename... Types>
    String BluetoothCommand(Type arg, Types... args) {
        Port.write(arg);
        return BluetoothCommand(args...);
    }
    
    String BluetoothCommand() {
        delay(1000);
        String Response = Port.readStringUntil('\n');
        Response.trim();
        return Response;
    }

    static unsigned long GetBaudCode(unsigned long baud) {
        switch (baud) {
        case 9600:
            return 4;
        case 38400:
            return 6;
        default:
            return 4;
        }
    }

    template <typename ConfigType>
    void OnSetup(const ConfigType& config) {
        SerialActivity<BluetoothHC06<DerivedType, _SerialType>, _SerialType>::OnSetup(config);
        OK = BluetoothCommand("AT") == "OK";
        if (OK) {
            //OK &= BluetoothCommand(String("AT+BAUD") + GetBaudCode(config.Baud)) == "OK" + config.Baud;
            OK &= BluetoothCommand(String("AT+NAME") + config.Name) == "OKsetname";
            OK &= BluetoothCommand(String("AT+PIN") + config.Password) == "OKsetPIN";
        }
    }

    void OnLoop(const ActivityContext& context) {
        if (OK) {
            SerialActivity<BluetoothHC06<DerivedType, _SerialType>, _SerialType>::OnLoop(context);
        }
    }

    bool IsBluetoothConnected() const {
        // TODO
        return OK && true;
    }
};
*/