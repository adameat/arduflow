#pragma once

struct Endl {};

template <typename DerivedType>
class WriteStream {
public:
    template <typename... Types>
    void write(Types... args);
    
    template <typename Type>
    void write(Type arg);
    
    template <typename Type, typename... Types>
    void write(Type arg, Types... args) {
        write(arg);
        write(args...);
    }

    template <typename... Types>
    void writeln(Types... args) {
        write(args...);
        write("\n");
    }

    void write(const char* arg) {
        write(AW::StringBuf(arg));
    }

    void write(const String& arg) {
        write(AW::StringBuf(arg.c_str(), arg.length()));
    }

    void write(unsigned long arg) {
        write(String(arg));
    }

    void write(double arg) {
        write(String(arg));
    }

    void write(float arg) {
        write(String(arg));
    }

    void write(AW::StringBuf arg) {
        static_cast<DerivedType*>(this)->send(arg);
    }

	WriteStream& operator <<(const char* arg) {
		write(arg);
		return *this;
	}

	WriteStream& operator <<(double arg) {
		write(arg);
		return *this;
	}

	WriteStream& operator <<(Endl) {
		write("\r\n");
		return *this;
	}
};


extern Endl endl;
