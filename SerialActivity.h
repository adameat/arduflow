#pragma once
class String;

#include <HardwareSerial.h>

template <typename DerivedType, typename _SerialType>
class SerialActivity : public Activity, public WriteStream<SerialActivity<DerivedType,_SerialType>> {
public:
    using SerialType = _SerialType;
    static constexpr int MaxIncomingSize = 256;
    static constexpr int MaxOutgoingSize = 256;
    SerialType Port;

	SerialActivity()
        : IncomingBufferSize(0)
		, OutgoingBufferSize(0)
	{}

    void OnReceived(const AW::StringBuf& buf) {
        static_cast<DerivedType*>(this)->OnSerialReceived(buf);
    }

    void Setup(int baud) {
    }
    
    void OnLoop(const ActivityContext&) {
	    int incomingSize = MaxIncomingSize - IncomingBufferSize;
		if (incomingSize > 0) {
			int availableForRead = Port.AvailableForRead();
			int size = incomingSize > availableForRead ? availableForRead : incomingSize;
			if (size > 0) {
                int strStart = 0;
                int bufferPos = IncomingBufferSize;
				size = Port.readBytes(&IncomingBuffer[IncomingBufferSize], size);
                IncomingBufferSize += size;
                while (bufferPos < IncomingBufferSize) {
                    if (IncomingBuffer[bufferPos] == '\n') {
                        int strSize = bufferPos;
                        while (strSize > 0 && IncomingBuffer[strSize - 1] == '\r') {
                            --strSize;
                        }
                        OnReceived(StringBuf(&IncomingBuffer[strStart], strSize));
                        strStart = bufferPos + 1;
                    }
                    ++bufferPos;
                }
                IncomingBufferSize = bufferPos - strStart;
                if (IncomingBufferSize > 0) {
                    memmove(IncomingBuffer, &IncomingBuffer[strStart], IncomingBufferSize);
                }
		    }
        }
        if (OutgoingBufferSize > 0) {
            int availableForWrite = Port.AvailableForWrite();
            int size = OutgoingBufferSize > availableForWrite ? availableForWrite : OutgoingBufferSize;
            if (size > 0) {
                size = Port.write(OutgoingBuffer, size);
                OutgoingBufferSize -= size;
                if (OutgoingBufferSize > 0) {
                    memmove(OutgoingBuffer, &OutgoingBuffer[size], OutgoingBufferSize);
                }
            }
        }
	}

    void send(const AW::StringBuf& str) {
        AW::StringBuf s(str);
        if (OutgoingBufferSize == 0) {
            int availableForWrite = Port.AvailableForWrite();
            int size = s.size() > availableForWrite ? availableForWrite : s.size();
            size = Port.write(s.data(), size);
            if (size == s.size())
                return;
            s = str.substr(size);
        }
        int sz = s.size();
        if (sz > MaxOutgoingSize - OutgoingBufferSize)
            sz = MaxOutgoingSize - OutgoingBufferSize;
        memmove(&OutgoingBuffer[OutgoingBufferSize], s.data(), sz);
        OutgoingBufferSize += sz;
    }

    bool IsAvailableForSend(int size) {
        return MaxOutgoingSize - OutgoingBufferSize >= size;
    }

protected:
	char IncomingBuffer[MaxIncomingSize];
	char OutgoingBuffer[MaxOutgoingSize];
	int IncomingBufferSize;
	int OutgoingBufferSize;
};

template <typename DerivedType, typename _SerialType>
class SerialConsole : public SerialActivity<SerialConsole<DerivedType, _SerialType>, _SerialType> {
public:
    SerialConsole<DerivedType, _SerialType>& Console;
    constexpr static bool OK = true;

    SerialConsole()
        : Console(*this)
    {}

    void Setup(int baud) {
        SerialActivity<SerialConsole<DerivedType, _SerialType>, _SerialType>::Setup(baud);
    }

    void OnSerialReceived(const AW::StringBuf& buf) {
        static_cast<DerivedType*>(this)->OnSerialReceived(buf);
    }
};
