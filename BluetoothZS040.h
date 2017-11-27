#pragma once

/*
template <typename DerivedType, typename _SerialType>
class BluetoothZS040 : public SerialActivity<BluetoothZS040<DerivedType, _SerialType>, _SerialType> {
public:
    BluetoothZS040<DerivedType, _SerialType>& Bluetooth;
    bool OK;
    bool Connected;

    BluetoothZS040()
        : Bluetooth(*this)
        , OK(false)
        , Connected(false)
    {}

    void OnBluetoothCommand(const StringBuf& buf) {}
    void OnBluetoothConnected() {}
    void OnBluetoothDisconnected() {}

    void OnSerialCommand(const StringBuf& buf) {
        if (!Connected) {
            if (buf == "CONNECTED") {
                Connected = true;
                static_cast<DerivedType*>(this)->OnBluetoothConnected();
                return;
            }
            if (buf.starts_with("+")) {
                return;
            }
        } else {
            if (buf == "+DISC:SUCCESS") {
                Connected = false;
                static_cast<DerivedType*>(this)->OnBluetoothDisconnected();
                return;
            }
        }
        static_cast<DerivedType*>(this)->OnBluetoothCommand(buf);
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
        String Response;
        Port.write("\r\n");
        do {
            Response = Port.readStringUntil('\n');
            Response.trim();
        } while (Response.length() != 0 && Response[0] == '+');
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

    void Setup(int baud) {
        SerialActivity<BluetoothZS040<DerivedType, _SerialType>, _SerialType>::Setup(baud);
        for (int i = 0; i < 3; ++i) {
            OK |= BluetoothCommand("AT") == "OK";
            if (OK)
                break;
        }
    }

    void Setup(int baud, const String& name, const String& password) {
        Setup(baud);
        if (OK) {
            //OK &= BluetoothCommand(String("AT+BAUD") + GetBaudCode(config.Baud)) == "OK" + config.Baud;
            OK &= BluetoothCommand(String("AT+NAME") + name) == "OK";
            OK &= BluetoothCommand(String("AT+PIN") + password) == "OK";
        }
    }

    void OnLoop(const ActivityContext& context) {
        if (OK) {
            SerialActivity<BluetoothZS040<DerivedType, _SerialType>, _SerialType>::OnLoop(context);
        }
    }

    bool IsBluetoothConnected() const {
        // TODO
        return OK && true;
    }
};
*/

namespace AW {

class TBluetoothZS040 {
protected:
    enum class EState {
        Error,
        AT,
        OK
    };

    EState State = EState::Error;
    TActor* Owner;
    TActor* Serial;
    bool Connected = false;
public:
    TBluetoothZS040(TActor* owner, TActor* serial)
        : Owner(owner)
        , Serial(serial) {}

    void Init(const TActorContext&) {
        //State = EState::AT;
        //context.Send(Owner, Serial, new TEventSerialData("AT"));
        State = EState::OK;
    }

    bool Receive(TUniquePtr<TEventSerialData>& event, const TActorContext&) {
        switch (State) {
        case EState::Error:
            break;
        case EState::OK:
            if (Connected) {
                if (event->Data.starts_with("+")) {
                    if (event->Data == "+DISC:SUCCESS") {
                        Connected = false;
                    }
                }
            } else
            if (event->Data == "CONNECTED") {
                Connected = true;
            }
            return false;
        case EState::AT:
            if (event->Data == "OK") {
                State = EState::OK;
            } else {
                State = EState::Error;
            }
        }
        return true;
    }

    void Receive(TUniquePtr<TEventReceive>&, const TActorContext&) {
    }

    bool IsOK() const {
        return State == EState::OK;
    }

    bool IsConnected() const {
        return Connected;
    }
};
}