#pragma once
#include <string.h>

namespace AW {

struct StringPointer {
	StringPointer(const char* ptr)
		: begin(ptr)
		, length(strlen(ptr))
	{}

	const char* begin;
	int length;
};

class StringBuf {
public:
	using size_type = unsigned int;
	constexpr static size_type npos = 0xff;

	constexpr StringBuf(const char* begin, size_type length)
		: Begin(begin)
		, End(begin + length) {}

	constexpr StringBuf(const char* begin, const char* end)
		: Begin(begin)
		, End(end) {}

	template <size_type N>
	constexpr StringBuf(const char(&string)[N])
		: StringBuf(&string[0], &string[N - 1])
	{}

	StringBuf(StringPointer ptr)
		: Begin(ptr.begin)
		, End(ptr.begin + ptr.length)
	{}

	constexpr StringBuf()
		: Begin(nullptr)
		, End(nullptr) {}

	size_type size() const {
		return End - Begin;
	}

	size_type length() const {
		return size();
	}

	bool empty() const {
		return Begin == End;
	}

	bool operator ==(const StringBuf& s) const {
		auto sz = size();
		return sz == s.size() && strncmp(Begin, s.Begin, sz) == 0;
	}

	const char* data() const {
		return Begin;
	}

	const char* begin() const {
		return Begin;
	}

	const char* end() const {
		return End;
	}

	StringBuf substr(size_type pos, size_type spos = npos) const {
		size_type sz = size();
		if (spos > sz - pos)
			spos = sz - pos;
		return StringBuf(Begin + pos, Begin + pos + spos);
	}

	bool starts_with(const StringBuf& s) const {
		size_type sz = s.size();
		return sz <= size() && strncmp(Begin, s.Begin, sz) == 0;
	}

	char operator [](size_type pos) const {
		return *(Begin + pos);
	}

	size_type find(char c) const {
		for (size_type p = 0; p < size(); ++p) {
			if ((*this)[p] == c) {
				return p;
			}
		}
		return npos;
	}

	operator int() const {
		return toint();
	}

	int toint() const {
		int v = 0;
		for (char c : *this) {
			if (c <= '9' && c >= '0') {
				v = v * 10 + (c - '0');
			} else {
				return int();
			}
		}
		return v;
	}

protected:
	const char* Begin;
	const char* End;
};

class String : public StringBuf {
public:
	static constexpr size_type npos = 0xff;

	constexpr String()
		: Buffer(nullptr)
	{}

	String(const char* begin, size_type length)
		: String()
	{
		assign(begin, length);
	}

	String(const char* begin, const char* end)
		: String(begin, end - begin)
	{}

	template <size_type N>
	constexpr String(const char(&string)[N])
		: StringBuf(&string[0], &string[N - 1])
		, Buffer(nullptr)
	{}

	String(StringPointer ptr)
		: String(ptr.begin, ptr.length)
	{}

	String(StringBuf buf)
		: String(buf.begin(), buf.size())
	{}

	String(unsigned int value, unsigned char base = 10)
		: String()
	{
		char buf[1 + 8 * sizeof(unsigned int)];
		utoa(value, buf, base);
		*this = StringPointer(buf);
	}

	String(int value, unsigned char base = 10)
		: String()
	{
		char buf[2 + 8 * sizeof(int)];
		itoa(value, buf, base);
		*this = StringPointer(buf);
	}

	String(unsigned long value, unsigned char base = 10)
		: String()
	{
		char buf[1 + 8 * sizeof(unsigned long)];
		ultoa(value, buf, base);
		*this = StringPointer(buf);
	}

	String(long value, unsigned char base = 10)
		: String()
	{
		char buf[2 + 8 * sizeof(long)];
		ltoa(value, buf, base);
		*this = StringPointer(buf);
	}

	String(float value, unsigned char decimalPlaces = 2)
		: String()
	{
		char buf[33];
		*this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, buf));
	}

	String(double value, unsigned char decimalPlaces = 2)
		: String()
	{
		char buf[33];
		*this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, buf));
	}

	String(const String& string)
		: StringBuf(string.Begin, string.End)
	{
		Buffer = string.Buffer;
		if (Buffer != nullptr) {
			++Buffer->RefCounter;
		}
	}

	String(String&& string) {
		Buffer = string.Buffer;
		Begin = string.Begin;
		End = string.End;
		string.Buffer = nullptr;
		string.Begin = nullptr;
		string.End = nullptr;
	}

	String& operator =(const String& string) {
		Free();
		Begin = string.Begin;
		End = string.End;
		Buffer = string.Buffer;
		if (Buffer != nullptr) {
			++Buffer->RefCounter;
		}
		return *this;
	}

	String& operator =(String&& string) {
		Free();
		Buffer = string.Buffer;
		Begin = string.Begin;
		End = string.End;
		string.Buffer = nullptr;
		string.Begin = nullptr;
		string.End = nullptr;
		return *this;
	}

	String& operator +=(const StringBuf& string) {
		append(string.data(), string.size());
		return *this;
	}

	String& operator +=(char string) {
		return *this += StringBuf(&string, 1);
	}

	String operator +(const StringBuf& string) {
		String result(*this);
		result += string;
		return result;
	}

	~String() {
		Free();
	}

	void assign(const char* string, size_type length) {
		resize(length);
		memcpy(Buffer->Data, string, length);
	}

	void append(const char* string, size_type length) {
		resize(size() + length);
		memcpy(const_cast<char*>(End) - length, string, length);
	}

	void erase(size_type pos, size_type length) {
		if (pos == 0 || pos + length == size()) {
			if (pos == 0) {
				Begin += length;
			} else {
				End -= length;
			}
		} else {
			String original = *this;
			resize(size() - length);
			if (pos != 0) {
				memcpy(Buffer->Data, original.begin(), pos);
			}
			if (pos + length != original.size()) {
				memcpy(Buffer->Data + pos, original.begin() + pos + length, original.size() - length - pos);
			}
		}
	}

	String substr(size_type pos, size_type length = npos) const {
		return String(*this, pos, length);
	}

	void reserve(size_type length) {
		EnsureOneOwner(length);
	}

	void resize(size_type length) {
		reserve(length);
		End = Begin + length;
	}

	void clear() {
		resize(0);
	}

	char* data() {
		EnsureOneOwner();
		return const_cast<char*>(Begin);
	}

	bool _IsShared() const { return Buffer == nullptr || Buffer->RefCounter > 1; }
	bool _IsUnique() const { return Buffer != nullptr && Buffer->RefCounter == 1; }

protected:
	void EnsureOneOwner(size_type size = 0) {
		if (Buffer != nullptr) {
			if (Buffer->RefCounter != 1) {
				StringBuf original = *this;
				size = max((size_type)(16 - sizeof(StringData)), max(size, original.size()));
				Free();
				Alloc(size);
				memcpy(Buffer->Data, original.data(), original.size());
				End = Begin + original.size();
			} else {
				if (Begin == End && Begin != Buffer->Data) {
					End = Begin = Buffer->Data;
				}
				if ((size_type)(Buffer->Length) - (Begin - Buffer->Data) < size) {
					Realloc(max((size_type)(16 - sizeof(StringData)), max(size, this->size())));
				}
			}
		} else {
			if (size != 0) {
				StringBuf original = *this;
				Alloc(size);
				memcpy(Buffer->Data, original.data(), original.size());
				End = Begin + original.size();
			}
		}
	}

	String(const String& string, size_type pos, size_type length)
		: String() {
		auto size = string.size();
		if (pos + length > size) {
			length = pos > size ? 0 : size - pos;
		}
		if (length != 0) {
			Buffer = string.Buffer;
			if (Buffer != nullptr) {
				++Buffer->RefCounter;
			}
			Begin = string.Begin + min(pos, size);
			End = Begin + length;
		}
	}

#ifndef ARDUINO
#pragma warning(disable:4200)
#endif
	struct StringData {
		size_type Length;
		unsigned char RefCounter;
		char Data[];
	};
#ifndef ARDUINO
#pragma warning(default:4200)
#endif

	void Alloc(size_type length) {
		Buffer = (StringData*)malloc(length + sizeof(StringData));
		Buffer->Length = length;
		Buffer->RefCounter = 1;
		Begin = Buffer->Data;
		End = Begin;
	}

	void Free() {
		if (Buffer != nullptr) {
			if (--Buffer->RefCounter == 0) {
				free(Buffer);
				Buffer = nullptr;
				Begin = End = nullptr;
			}
		}
	}

	void Realloc(size_type length) {
		StringData* newBuffer = (StringData*)realloc(Buffer, length + sizeof(StringData));
		if (newBuffer == Buffer) {
			Buffer->Length = length;
		} else {
			size_type size = this->size();
			Buffer = newBuffer;
			Buffer->RefCounter = 1;
			Buffer->Length = length;
			Begin = Buffer->Data;
			End = Begin + size;
		}
	}

	StringData* Buffer;
};

class StringStream {
public:
	/*StringStream& operator <<(String string) {
		Data += string;
		return *this;
	}*/

	StringStream& operator <<(StringBuf string) {
		Data += string;
		return *this;
	}

	StringStream& operator <<(char string) {
		Data += string;
		return *this;
	}

	StringStream& operator <<(int string) {
		Data += String(string);
		return *this;
	}

	StringStream& operator <<(unsigned int string) {
		Data += String(string);
		return *this;
	}

	StringStream& operator <<(long string) {
		Data += String(string);
		return *this;
	}

	StringStream& operator <<(unsigned long string) {
		Data += String(string);
		return *this;
	}

	StringStream& operator <<(float string) {
		Data += String(string);
		return *this;
	}

	StringStream& operator <<(double string) {
		Data += String(string);
		return *this;
	}

	operator String() const {
		return Data;
	}

	void clear() {
		Data.clear();
	}

	String::size_type size() const {
		return Data.size();
	}

protected:
	String Data;
};

}
