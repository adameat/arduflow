#pragma once

#include <HardwareSerial.h>
#include <SoftwareSerial.h>

namespace AW {

template <HardwareSerial& Port, long Baud>
class THardwareSerial {
public:
    void Begin() {
        Port.begin(Baud);
    }

    int AvailableForRead() const {
        return Port.available();
    }

    int AvailableForWrite() const {
        return Port.availableForWrite();
    }

    int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }
};

template <int RxPin, int TxPin, long Baud>
class TSoftwareSerial {
protected:
    SoftwareSerial Port;

public:
    TSoftwareSerial()
        : Port(RxPin, TxPin)
    {}

    void Begin() {
        Port.begin(Baud);
    }

    int AvailableForRead() const {
        return const_cast<SoftwareSerial&>(Port).available();
    }

    int AvailableForWrite() const {
        return 32;
    }

    int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }
};


struct TEventSerialData : TBasicEvent<TEventSerialData> {
    constexpr static TEventID EventID = 2; // TODO
    String Data;

    TEventSerialData(String data)
        : Data(data) {}
};

template <typename SerialType>
class TSerialActor : public TActor {
    static constexpr unsigned int MaxBufferSize = 256;
public:
    TSerialActor(TActor* owner)
        : Owner(owner)
        , EOL("\n")
    {}

protected:
    SerialType Port;
    TActor* Owner;
    String Buffer;
    StringBuf EOL;

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
        Port.Begin();
        context.Send(this, this, new TEventReceive);
    }

    void OnSerialData(TUniquePtr<TEventSerialData> event, const TActorContext& context) {
        int availableForWrite = Port.AvailableForWrite();
        int len = event->Data.length();
        int size = len > availableForWrite ? availableForWrite : len;
        if (size > 0) {
            Port.Write(event->Data.begin(), size);
            if (size < len) {
                event->Data.erase(0, size);
                context.ResendImmediate(this, event.Release());
            } else {
                // TODO: could block
                Port.Write(EOL.data(), EOL.size());
            }
        } else {
            context.ResendImmediate(this, event.Release());
        }
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        auto size = min((unsigned int)Port.AvailableForRead(), MaxBufferSize - Buffer.size());
        if (size > 0) {
            //::Serial.println(size);
            int strStart = 0;
            auto bufferPos = Buffer.size();
            Buffer.reserve(Buffer.size() + size);
            size = Port.Read(Buffer.data() + Buffer.size(), size);
            Buffer.resize(Buffer.size() + size);
            while (bufferPos < Buffer.size()) {
                if (Buffer[bufferPos] == '\n') {
                    int strSize = bufferPos;
                    while (strSize > 0 && Buffer[strSize - 1] == '\r') {
                        --strSize;
                    }
                    //::Serial.println(Buffer.substr(strStart, strSize - strStart).data());
                    context.Send(this, Owner, new TEventSerialData(Buffer.substr(strStart, strSize - strStart)));
                    strStart = bufferPos + 1;
                }
                ++bufferPos;
            }
            Buffer = Buffer.substr(strStart, bufferPos - strStart);
        }
        context.Resend(this, event.Release());
    }
};

}
