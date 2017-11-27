#pragma once

#include <Arduino.h>
#include <avr/eeprom.h>
#include <Wire.h>

namespace AW {

class ArduinoSettings {
public:
    static constexpr float GetReferenceVoltage() {
#if F_CPU == 8000000L
		return 3.3;
#else
        return 5.0; // TODO: for now
#endif
    }

	static constexpr int GetReadResolution() {
        return 1024; // TODO: for now
    }

	static constexpr int GetWriteResolution() {
        return 256; // TODO: for now
    }
};

template <typename Type>
inline Type&& Move(Type&& value) {
	return static_cast<Type&&>(value);
}

template <typename Type>
struct TUniquePtr {
	TUniquePtr(Type* ptr = nullptr)
		: Ptr(ptr)
	{}

	TUniquePtr(TUniquePtr<Type>& ptr) {
		Ptr = ptr.Ptr;
		ptr.Ptr = nullptr;
	}

	TUniquePtr(TUniquePtr<Type>&& ptr) {
		Ptr = ptr.Ptr;
		ptr.Ptr = nullptr;
	}

	~TUniquePtr() {
		delete Ptr;
	}

	TUniquePtr<Type>& operator =(Type* ptr) {
		delete Ptr;
		Ptr = ptr;
		return *this;
	}

	TUniquePtr<Type>& operator =(TUniquePtr<Type>& ptr) {
		TUniquePtr<Type> _this = *this;
		Ptr = ptr.Ptr;
		ptr.Ptr = nullptr;
		return *this;
	}

	TUniquePtr<Type>& operator =(TUniquePtr<Type>&& ptr) {
		delete Ptr;
		Ptr = ptr.Ptr;
		ptr.Ptr = nullptr;
		return *this;
	}

	Type* operator ->() const {
		return Ptr;
	}

	Type& operator *() const {
		return *Ptr;
	}

	Type* Get() const {
		return Ptr;
	}

	Type* Release() {
		Type* ptr = Ptr;
		Ptr = nullptr;
		return ptr;
	}

protected:
	Type* Ptr;
};

template <typename ItemType, int Capacity>
class TVector {
public:
	using Iterator = ItemType*;

	TVector()
		: End(begin())
	{}

	int size() const {
		return end() - begin();
	}

	bool empty() const {
		return begin() == end();
	}

	Iterator begin() {
		return reinterpret_cast<ItemType*>(&Data);
	}

	Iterator end() {
		return End;
	}

	void push_back(const ItemType& item) {
		insert(end(), item);
	}

	void push_front(const ItemType& item) {
		insert(begin(), item);
	}

	Iterator erase(Iterator it) {
		it->~ItemType();
		while (it != end()) {
			Iterator next = it;
			++next;
			*it = *next;
			it = next;
		}
		--End;
		return it;
	}

	Iterator insert(Iterator it, const ItemType& item) {
		if (end() < capacity_end()) {
			if (it < end()) {
				Iterator to = end();
				while (to >= it) {
					Iterator from = to;
					--from;
					*to = *from;
					to = from;
				}
			}
			new (&*it) ItemType(item);
			++End;
		}
		return it;
	}

protected:
	Iterator capacity_end() {
		return reinterpret_cast<ItemType*>(&Data[sizeof(ItemType) * Capacity]);
	}

	char Data[sizeof(ItemType) * Capacity];
	Iterator End;
};

template <typename ItemType, int Capacity>
class TDeque {};

template <typename ItemType, int Capacity>
class TDeque<TUniquePtr<ItemType>, Capacity> {
public:
	using TItemType = TUniquePtr<ItemType>;
	using Iterator = TItemType*;

	TDeque()
		: BeginIdx(0)
		, EndIdx(0)
	{}

	int size() const {
		return EndIdx - BeginIdx;
	}

	int capacity() const {
		return Capacity;
	}

	bool empty() const {
		return BeginIdx == EndIdx;
	}

	Iterator begin() {
		return capacity_begin() + BeginIdx;
	}

	Iterator end() {
		return capacity_begin() + EndIdx;
	}

	TItemType& front() {
		return *begin();
	}

	void push_back(TItemType item) {
		insert(end(), item);
	}

	void push_front(TItemType item) {
		insert(begin(), item);
	}

	void pop_front() {
		*begin() = TItemType();
		if (++BeginIdx == EndIdx) {
			BeginIdx = EndIdx = 0;
		}
	}

	Iterator erase(Iterator it) {
		*it = nullptr;
		if (it == begin()) {
			++BeginIdx;
			++it;
		} else if (it == end() - 1) {
			--EndIdx;
		} else {
			// TODO: check the distance
			move(it + 1, end(), it);
			--EndIdx;
		}
		if (BeginIdx == EndIdx) {
			BeginIdx = EndIdx = 0;
			it = begin();
		}
		return it;
	}

	Iterator insert(Iterator it, TItemType item) {
		if (size() < capacity()) {
			if (it == begin()) {
				if (it > capacity_begin()) {
					--BeginIdx;
					*begin() = item;
					return begin();
				}
				move(it, end(), it + 1);
				++EndIdx;
				*it = item;
			} else
			if (it == end()) {
				if (it < capacity_end()) {
					*end() = item;
					++EndIdx;
					return it;
				}
				move(begin(), it, begin() - 1);
				--BeginIdx;
				--it;
				*it = item;
			}
		} else {
			Serial.print("\nOVERFLOW!\n");
		}
		return it;
	}

protected:
	void move(Iterator begin, Iterator end, Iterator to) {
		if (begin == end)
			return;
		if (to < begin) {
			Iterator it = begin;
			while (it != end) {
				*to = Move(*it);
				++to;
				++it;
			}
		} else {
			Iterator it = end;
			to += (end - begin);
			do {
				--to;
				--it;
				*to = Move(*it);
			} while (it != begin);
		}
	}

	Iterator capacity_begin() {
		return &Data[0];
	}

	Iterator capacity_end() {
		return &Data[Capacity];
	}

	TItemType Data[Capacity];
#ifndef ARDUINO
#pragma warning(disable:4201)
#endif
	struct {
		int BeginIdx : 4;
		int EndIdx : 4;
	};
#ifndef ARDUINO
#pragma warning(default:4201)
#endif
};

template <typename ItemType>
class TDeque<TUniquePtr<ItemType>, 0> {};
//public:
//	using TItemType = TUniquePtr<ItemType>;
//	using Iterator = TItemType*;
//
//	TDeque()
//		: Data(nullptr) {}
//
//	int size() const {
//		return End - Begin;
//	}
//
//	bool empty() const {
//		return Begin == End;
//	}
//
//	Iterator begin() {
//		return Begin;
//	}
//
//	Iterator end() {
//		return End;
//	}
//
//	TItemType& front() {
//		return *begin();
//	}
//
//	void push_back(TItemType item) {
//		insert(end(), item);
//	}
//
//	void push_front(TItemType item) {
//		insert(begin(), item);
//	}
//
//	void pop_front() {
//		*Begin = TItemType();
//		if (++Begin == End) {
//			Begin = End = capacity_begin();
//		}
//	}
//
//	Iterator erase(Iterator it) {
//		*it = nullptr;
//		while (it != end()) {
//			Iterator next = it;
//			++next;
//			*it = *next;
//			it = next;
//		}
//		--End;
//		if (Begin == End) {
//			it = Begin = End = capacity_begin();
//		}
//		return it;
//	}
//
//	Iterator insert(Iterator it, TItemType item) {
//		if (end() < capacity_end()) {
//			if (it == begin() && it > capacity_begin()) {
//				--Begin;
//				*Begin = item;
//				return Begin;
//			}
//			if (it < end()) {
//				Iterator to = end();
//				while (to >= it) {
//					Iterator from = to;
//					--from;
//					*to = *from;
//					to = from;
//				}
//			}
//			*it = item;
//			++End;
//		}
//		return it;
//	}
//
//protected:
//	Iterator capacity_begin() {
//		return &Data[0];
//	}
//
//	Iterator capacity_end() {
//		return &Data[Capacity];
//	}
//
//	TItemType Data[Capacity];
//	Iterator Begin;
//	Iterator End;
//};

template <typename ItemType>
class TList {};

template <typename ItemType>
class TList<TUniquePtr<ItemType>> {
public:
	using TItemType = TUniquePtr<ItemType>;

	class TItemBase {
		friend TList;
	private:
		TUniquePtr<ItemType> Next;
	};
	
	class Iterator {
		friend TList;
	public:
		Iterator(ItemType* item)
			: Item(item)
		{}

		bool operator ==(const Iterator& iterator) {
			return Item == iterator.Item;
		}

		bool operator !=(const Iterator& iterator) {
			return Item != iterator.Item;
		}

		Iterator& operator ++() {
			Item = Item->Next.Get();
			return *this;
		}

		Iterator operator ++(int) {
			Iterator prev(Item);
			operator ++();
			return prev;
		}

		ItemType* Get() {
			return Item;
		}

	protected:
		ItemType* Item;
	};

	int size() {
		int s = 0;
		Iterator it = begin();
		while (it != end()) {
			++s;
			++it;
		}
		return s;
	}

	bool empty() {
		return begin() == end();
	}

	Iterator begin() {
		return Iterator(Begin.Get());
	}

	Iterator end() {
		return Iterator(nullptr);
	}

	const TItemType& front() {
		return Begin;
	}

	Iterator push_back(TItemType item) {
		return insert(end(), item);
	}

	Iterator push_front(TItemType item) {
		return insert(begin(), item);
	}

	Iterator pop_front() {
		return erase(begin());
	}

	TItemType pop_value(Iterator& it) {
		if (it == begin()) {
			TItemType value(Begin);
			it = (Begin = value->Next).Get();
			return value;
		} else {
			Iterator next = begin();
			while (next != end()) {
				Iterator prev = next++;
				if (next == it) {
					TItemType value(prev.Get()->Next);
					it = (prev.Get()->Next = value->Next).Get();
					return value;
				}
			}
		}
		return TItemType();
	}

	Iterator erase(Iterator it) {
		if (it == begin()) {
			Begin = Begin->Next;
			return Begin.Get();
		} else {
			Iterator next = begin();
			while (next != end()) {
				Iterator prev = next++;
				if (next == it) {
					return (prev.Get()->Next = next.Get()->Next).Get();
				}
			}
		}
		return end();
	}

	Iterator insert(Iterator it, TItemType item) {
		if (it == begin()) {
			item->Next = Begin;
			Begin = item;
			return begin();
		} else {
			Iterator next = begin();
			while (next != end()) {
				Iterator prev = next++;
				if (next == it) {
					item->Next = prev.Get()->Next;
					prev.Get()->Next = item;
					return next;
				}
			}
		}
		return end();
	}

protected:
	TUniquePtr<ItemType> Begin;
};

using TEventID = unsigned int;
class String;

class TTime {
public:
	constexpr TTime()
		: Value() {}

	bool operator ==(TTime time) const { return Value == time.Value; }
	bool operator <(TTime time) const { return Value < time.Value; }
	bool operator <=(TTime time) const { return Value <= time.Value; }
	bool operator >(TTime time) const { return Value > time.Value; }
	bool operator >=(TTime time) const { return Value >= time.Value; }
	TTime operator +(TTime time) const { return TTime(Value + time.Value); }
	TTime operator -(TTime time) const { return TTime(Value - time.Value); }
	static constexpr TTime MilliSeconds(unsigned long ms) { return TTime(ms); }
	static constexpr TTime Seconds(unsigned long s) { return TTime(s * 1000); }
	static TTime Now() { return TTime(millis()); }
	String AsString() const;
	constexpr unsigned long MilliSeconds() const { return Value; }
	static constexpr TTime Max() { return TTime(-1); }
	static constexpr TTime Zero() { return TTime(0); }

protected:
	constexpr TTime(unsigned long ms)
		: Value(ms) {}

	unsigned long Value;
};

class TActor;
struct TEvent;
class TActorLib;

using TActorPtr = TActor*;
using TEventPtr = TUniquePtr<TEvent>;

struct TActorContext {
	TActorLib& ActorLib;
	TTime Now;

	TActorContext(TActorLib& actorLib)
		: ActorLib(actorLib)
		, Now(TTime::Now())
	{}

	void Send(TActor* sender, TActor* recipient, TEventPtr event) const;
	void SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) const;
	void Resend(TActor* recipient, TEventPtr event) const;
	void ResendImmediate(TActor* recipient, TEventPtr event) const;
};

class TActor {
private:
	friend class TActorLib;
	TActor* NextActor = nullptr;
	//TDeque<TEventPtr, 9> Events;
	TList<TEventPtr> Events;
public:
	virtual void OnEvent(TEventPtr event, const TActorContext& context) = 0;
};

struct TEvent : TList<TUniquePtr<TEvent>>::TItemBase {
	TTime NotBefore;
	TActor* Sender;
	//TActor* Recipient;
	TEventID EventID;
};

template <typename DerivedType>
struct TBasicEvent : TEvent {
	TBasicEvent() {
		TEvent::EventID = DerivedType::EventID;
	}
};

struct TEventBootstrap : TBasicEvent<TEventBootstrap> {
	constexpr static TEventID EventID = 0; // TODO
};

struct TEventReceive : TBasicEvent<TEventReceive> {
	constexpr static TEventID EventID = 1; // TODO

	TEventReceive() = default;

	TEventReceive(TTime notBefore) {
		NotBefore = notBefore;
	}
};

class TActorLib {
public:
	TActorLib();
	void Register(TActor* actor);
	void Run();
	void Send(TActor* sender, TActor* recipient, TEventPtr event);
	void SendImmediate(TActor* sender, TActor* recipient, TEventPtr event);
	void Resend(TActor* recipient, TEventPtr event);
	void ResendImmediate(TActor* recipient, TEventPtr event);

protected:
	//TDeque<TEventPtr, 16> Events;
	
	TActor* Actors;
	// TDeque<TEventPtr> with different sizes in every actor
	// or maybe dynamic TDeque<TEventPtr> ?
	// mailbox should be inside every actor for faster sending
};

template <typename Type, int WindowSize = 10>
class TAverage {
public:
	TAverage()
		: Accumulator()
		, Count()
	{}

	void AddValue(Type value) {
		Accumulator += value;
		Count += 1;
		if (Count >= WindowSize * 2) {
			Accumulator /= 2;
			Count /= 2;
		}
	}

	Type GetValue() const {
		return Accumulator / Count;
	}

protected:
	Type Accumulator;
	int Count;
};

template <typename Type>
class TAverage<Type, 0> {
public:
	TAverage()
		: Accumulator()
	{}

	void AddValue(Type value) {
		Accumulator = value;
	}

	Type GetValue() const {
		return Accumulator;
	}

protected:
	Type Accumulator;
};

template <uint8_t P, uint8_t Mode = OUTPUT>
class TPin {
public:
	TPin() {
		pinMode(P, Mode);
	}

	TPin& operator =(bool state) {
		digitalWrite(P, state ? HIGH : LOW);
		return *this;
	}

	operator bool() const {
		return digitalRead(P) == HIGH;
	}

	TPin& operator =(int value) {
		analogWrite(P, value);
		return *this;
	}

	operator int() const {
		return analogRead(P);
	}

	float GetValue() const {
		int value = analogRead(P);
		return ArduinoSettings::GetReferenceVoltage() * value / (ArduinoSettings::GetReadResolution() - 1);
	}

	template <int Iterations = 1000, unsigned long Delay = 0>
	float GetAveragedValue() const {
		TAverage<float, Iterations> value;
		for (int i = 0; i < Iterations; ++i) {
			if (i != 0) {
				delay(Delay);
			}
			value.AddValue(GetValue());
		}
		return value.GetValue();
	}

	void SetValue(float value) const {
		int v = value * (ArduinoSettings::GetWriteResolution() - 1);
		analogWrite(P, v);
	}

	void SetMode(uint8_t mode) {
		pinMode(P, mode);
	}
};

template <typename Type, int WindowSize = 10>
class TAveragedValue {
protected:
	Type Value;
	using ValueType = decltype(Value.GetValue());	
	mutable TAverage<ValueType, WindowSize> Average;

public:
	ValueType GetValue() const {
		Average.AddValue(Value.GetValue());
		return Average.GetValue();
	}
};

class TPeriodicTrigger {
public:
	bool IsTriggered(TTime period, const TActorContext& context) {
		if (LastTriggered + period <= context.Now) {
			LastTriggered = context.Now;
			return true;
		}
		return false;
	}

protected:
	TTime LastTriggered;
};

template <typename StructType>
class TEEPROM {
public:
	template <typename ValueType>
	ValueType Get(ValueType StructType::* ptr) const {
		int idx = reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0);
		ValueType value;
		Get(idx, value);
		return value;
	}

	template <typename ValueType>
	void Put(ValueType StructType::* ptr, ValueType val) const {
		int idx = reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0);
		Put(idx, val);
	}

	template <typename ValueType>
	void Update(ValueType StructType::* ptr, ValueType val) const {
		int idx = reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0);
		Update(idx, val);
	}

protected:
	template <typename T>
	static void Get(int idx, T& val) {
		for (size_t i = 0; i < sizeof(T); ++i) {
			((uint8_t*)&val)[i] = eeprom_read_byte(reinterpret_cast<uint8_t*>(idx + i));
		}
	}

	template <typename T>
	static void Put(int idx, const T& val) {
		for (size_t i = 0; i < sizeof(T); ++i) {
			eeprom_write_byte(reinterpret_cast<uint8_t*>(idx + i), ((uint8_t*)&val)[i]);
		}
	}

	template <typename T>
	static void Update(int idx, const T& val) {
		for (size_t i = 0; i < sizeof(T); ++i) {
			if (eeprom_read_byte(reinterpret_cast<uint8_t*>(idx + i)) != ((uint8_t*)&val)[i]) {
				eeprom_write_byte(reinterpret_cast<uint8_t*>(idx + i), ((uint8_t*)&val)[i]);
			}
		}
	}
};

class TWire {
public:
	static void Begin() { Wire.begin(); }
	static void BeginTransmission(uint8_t address) { Wire.beginTransmission(address); }
	static void Write(uint8_t value) { Wire.write(value); }
	static void Write(uint16_t value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Wire.write(values[1]); Wire.write(values[0]); }
	static bool EndTransmission(bool stop = true) { return Wire.endTransmission(stop) == 0; }
	static uint8_t RequestFrom(uint8_t address, uint8_t quantity) { return Wire.requestFrom(address, quantity); }
	static void Read(uint8_t& value) { value = Wire.read(); }
	static void Read(int8_t& value) { Read(reinterpret_cast<uint8_t&>(value)); }
	static void Read(uint16_t& value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Read(values[1]); Read(values[0]); }
	static void Read(int16_t& value) { Read(reinterpret_cast<uint16_t&>(value)); }
	static void ReadLE(uint16_t& value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Read(values[0]); Read(values[1]); }
	static void ReadLE(int16_t& value) { ReadLE(reinterpret_cast<uint16_t&>(value)); }

	template <typename T>
	static bool ReadValue(uint8_t addr, uint8_t reg, T& val) {
		BeginTransmission(addr);
		Write(reg);
		if (!EndTransmission())
			return false;
		if (RequestFrom(addr, sizeof(T)) != sizeof(T))
			return false;
		Read(val);
		return true;
	}

	template <typename T>
	static bool ReadValueLE(uint8_t addr, uint8_t reg, T& val) {
		BeginTransmission(addr);
		Write(reg);
		if (!EndTransmission())
			return false;
		if (RequestFrom(addr, sizeof(T)) != sizeof(T))
			return false;
		ReadLE(val);
		return true;
	}

	template <typename T>
	static bool WriteValue(uint8_t addr, uint8_t reg, T& val) {
		BeginTransmission(addr);
		Write(reg);
		Write(val);
		return EndTransmission();
	}

	template <typename T>
	static void Read(T& value) {
		uint8_t* data = reinterpret_cast<uint8_t*>(&value);
		auto length = sizeof(value);
		while (length-- > 0) {
			Read(*data);
			++data;
		}
	}
};

struct TDefaultEnvironment {
	// debug info
	static constexpr bool Diagnostics = false;

	// sensors
	static constexpr TTime SensorsPeriod = TTime::MilliSeconds(5000);
	static constexpr bool SensorsSendValues = true;
	static constexpr bool SensorsCalibration = false;

	using Wire = TWire;
};

} // namespace AW

//struct ActivityContext {
//    unsigned long Now;
//
//    ActivityContext()
//        : Now(millis())
//    {}
//};
//
//template <unsigned long Period>
//class PeriodicTrigger {
//public:
//    PeriodicTrigger() {
//        NextTick = Period;
//    }
//
//    bool IsTriggered(const ActivityContext& context) {
//        if (context.Now > NextTick) {
//            // TODO: overflow trick
//            NextTick += Period;
//            return true;
//        } else {
//            return false;
//        }
//    }
//
//protected:
//    unsigned long NextTick;
//};

//template <typename Derived, unsigned long Period>
//class PeriodicActivity : public Activity {
//public:
//    void OnLoop(const ActivityContext& context) {
//        if (Trigger.IsTriggered(context)) {
//            static_cast<Derived*>(this)->OnPeriod();
//        }
//    }
//    
//protected:
//    PeriodicTrigger<Period> Trigger;
//};

//template <uint8_t P, uint8_t Mode = OUTPUT>
//class Pin {
//public:
//    Pin() {
//        pinMode(P, Mode);
//    }
//
//    Pin& operator =(bool state) {
//        digitalWrite(P, state ? HIGH : LOW);
//        return *this;
//    }
//
//    operator bool() const {
//        return digitalRead(P) == HIGH;
//    }
//
//    Pin& operator =(int value) {
//        analogWrite(P, value);
//        return *this;
//    }
//
//    operator int() const {
//        return analogRead(P);
//    }
//
//    float GetValue() const {
//        int value = analogRead(P);
//        return ArduinoSettings::GetReferenceVoltage() * value / ArduinoSettings::GetReadResolution();
//    }
//
//    void SetValue(float value) const {
//        int v = value * ArduinoSettings::GetWriteResolution();
//        analogWrite(P, v);
//    }
//};

//class TWireWriteSession {
//public:
//	TWireWriteSession(uint8_t Address) {
//		TWire::BeginTransmission(Address);
//	}
//
//	~TWireWriteSession() {
//		TWire::EndTransmission();
//	}
//
//	TWireWriteSession& operator <<(uint8_t value) {
//		TWire::Write(value);
//		return *this;
//	}
//};
//
//template <uint8_t Address, typename... ValueTypes>
//class TWireReadSession {
//	ValueTypes&... Values;
//public:
//	TWireReadSession(ValueTypes&... values)
//};
//
//template <uint8_t Address>
//class TWire {
//public:
//	template <typename Type>
//	TWireWriteSession operator <<(Type value) { return TWireWriteSession(Address) << value; }
//
//	template <typename Type>
//	TWireReadSession<Address, Type> operator >>(Type& value) { return TWireReadSession<Address, Type>(value); }
//};

//template <int P>
//class LedActivity : public Activity {
//protected:
//    LedActivity<P>& Led;
//    
//public:
//    LedActivity()
//        : Led(*this)
//    {
//        Off();
//    }
//    
//    void OnLoop(const ActivityContext& context) {
//        
//    }
//
//    void On() {
//        LedPin = true;
//    }
//
//    void Off() {
//        LedPin = false;
//    }
//    
//private:
//    Pin<P> LedPin;
//};

#include "StringBuf.h"
#include "Stream.h"
#include "Serial.h"
#include "Bluetooth.h"
#include "Display.h"
#include "Led.h"
#include "Sensors.h"

//template <typename...>
//class TArduinoChain;
//
//template <typename ActivityType>
//class TArduinoChain<ActivityType> : public ActivityType {
//public:
//    void Loop(const ActivityContext& context) {
//        ActivityType::OnLoop(context);
//    }
//};
//
//template <typename ActivityType, typename... ActivityTypes>
//class TArduinoChain<ActivityType, ActivityTypes...> : public TArduinoChain<ActivityType>, public TArduinoChain<ActivityTypes...> {
//public:
//    void Loop(const ActivityContext& context) {
//        TArduinoChain<ActivityType>::Loop(context);
//        TArduinoChain<ActivityTypes...>::Loop(context);
//    }
//};
//
//template <typename... ActivityTypes>
//class TArduinoWorkflow : public TArduinoChain<ActivityTypes...> {
//public:
//    void Loop() {
//        ActivityContext context;
//        TArduinoChain<ActivityTypes...>::Loop(context);
//    }
//};
//
