#pragma once

/*
template <typename DerivedType, typename _SerialType = HardwareSerial, byte PIN_POWER = 50, byte PIN_STATE = 51, byte PIN_34 = 34>
class BluetoothHC05 : public SerialActivity<BluetoothHC05<DerivedType, _SerialType, PIN_POWER, PIN_STATE, PIN_34>, _SerialType> {
protected:
	enum class EInitState {
		Failed,
		AT,
		Name,
		Role,
		Password,
		Baud,
		Done
	};

	EInitState InitState = EInitState::Failed;
public:
	String Name;
	String Password;
	int Baud = 0;
    bool OK = false;

	void OnSerialReceived(const StringBuf& buf) {
		switch (InitState) {
			case EInitState::Failed:
				return;
			case EInitState::AT: {
				if (buf == "OK") {

				}
			}
		}
        static_cast<DerivedType*>(this)->OnBluetoothReceived(buf);
    }

    template <typename... Types>
    String BluetoothCommand(Types... args);
    
    template <typename Type>
    String BluetoothCommand(Type arg);
    
    template <typename Type, typename... Types>
    String BluetoothCommand(Type arg, Types... args) {
        write(arg);
        return BluetoothCommand(args...);
    }

    String BluetoothCommand(const char* Command) {
        write(Command);
        write("\r\n");
        String Response = readStringUntil('\n');
        Response.trim();
        return Response;
    }

    template <typename ConfigType>
    void OnSetup(const ConfigType& config) {
        SerialActivity<BluetoothHC05<DerivedType, _SerialType, PIN_POWER, PIN_STATE, PIN_34>, _SerialType>::OnSetup(config);
        pinMode(PIN_POWER, OUTPUT);
        pinMode(PIN_STATE, INPUT_PULLUP);
        pinMode(PIN_34, OUTPUT);

        digitalWrite(PIN_POWER, LOW);
        digitalWrite(PIN_34, LOW);
        delay(500);
        // set pin 34 HIGH
        digitalWrite(PIN_34, HIGH);
        delay(100);
        // turn on the HC-05
        digitalWrite(PIN_POWER, HIGH); 
        delay(1000);
		write("AT\r\n");
		InitState = EInitState::AT;

        OK = BluetoothCommand("AT") == "OK";
        if (OK) {
            OK &= BluetoothCommand("AT+NAME=", config.Name) == "OK";
            OK &= BluetoothCommand("AT+ROLE=0") == "OK";
            OK &= BluetoothCommand("AT+PSWD=", config.Password) == "OK";
            OK &= BluetoothCommand("AT+UART=", config.Baud, ",0,0") == "OK";

            if (OK) {
                digitalWrite(PIN_34, LOW);
                digitalWrite(PIN_POWER, LOW);
                delay(1000);
                digitalWrite(PIN_POWER, HIGH);
            }
        }
    }

    void OnLoop(const ActivityContext& context) {
        if (OK) {
            SerialActivity<BluetoothHC05<DerivedType, _SerialType, PIN_POWER, PIN_STATE, PIN_34>, _SerialType>::OnLoop(context);
        }
    }

    bool IsBluetoothConnected() const {
        return digitalRead(PIN_STATE) != LOW;
    }
};
*/

namespace AW {

template <byte PIN_POWER = 50, byte PIN_STATE = 51, byte PIN_34 = 34>
class TBluetoothHC05 {
protected:
	enum class EState {
		Error,
		Pin34,
		PinPower,
		Init,
		AT,
		Name,
		Role,
		Password,
		Baud,
		Reset,
		OK
	};

	EState State = EState::Error;
	TActor* Owner = nullptr;
	TActor* Serial = nullptr;

public:
	TBluetoothHC05(TActor* owner, TActor* serial)
		: Owner(owner)
		, Serial(serial)
	{}

	void Init(const TActorContext& context) {
		pinMode(PIN_POWER, OUTPUT);
		pinMode(PIN_STATE, INPUT_PULLUP);
		pinMode(PIN_34, OUTPUT);
		digitalWrite(PIN_POWER, LOW);
		digitalWrite(PIN_34, LOW);
		State = EState::Pin34;
		context.Send(Owner, Owner, new TEventReceive(context.Now + TTime::MilliSeconds(500)));
		//context.Send(Owner, Serial, new TEventSerialData("AT"));
	}

	bool Receive(TUniquePtr<TEventSerialData>& event, const TActorContext& context) {
		switch (State) {
		case EState::OK:
			return false;
		case EState::Error:
			break;
		case EState::AT:
			if (event->Data == "OK") {
				
				//context.Send(Owner, Serial, new TEventSerialData("AT+NAME="));
				/*OK &= BluetoothCommand("AT+NAME=", config.Name) == "OK";
				OK &= BluetoothCommand("AT+ROLE=0") == "OK";
				OK &= BluetoothCommand("AT+PSWD=", config.Password) == "OK";
				OK &= BluetoothCommand("AT+UART=", config.Baud, ",0,0") == "OK";*/
				digitalWrite(PIN_34, LOW);
				digitalWrite(PIN_POWER, LOW);
				State = EState::Reset;
				context.Send(Owner, Owner, new TEventReceive(context.Now + TTime::MilliSeconds(1000)));
			} else {
				State = EState::Error;
			}
		}
		return true;
	}

	void Receive(TUniquePtr<TEventReceive>& event, const TActorContext& context) {
		switch (State) {
		case EState::Pin34:
			// set pin 34 HIGH
			digitalWrite(PIN_34, HIGH);
			State = EState::PinPower;
			context.Send(Owner, Owner, new TEventReceive(context.Now + TTime::MilliSeconds(100)));
			return;
		case EState::PinPower:
			// turn on the HC-05
			digitalWrite(PIN_POWER, HIGH);
			State = EState::Init;
			context.Send(Owner, Owner, new TEventReceive(context.Now + TTime::MilliSeconds(1000)));
			return;
		case EState::Init:
			State = EState::AT;
			context.Send(Owner, Serial, new TEventSerialData("AT"));
			return;
		case EState::Reset:
			digitalWrite(PIN_POWER, HIGH);
			State = EState::OK;
			return;
		}
		State = EState::Error;
	}

	bool IsOK() const {
		return State == EState::OK;
	}

	bool IsConnected() const {
		return digitalRead(PIN_STATE) != LOW;
	}
};

}