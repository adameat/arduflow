#if defined(__AVR_ATmega2560__)
#include "DisplayActivitySPFD5408.h"
class DisplayActivity : public DisplayActivitySPFD5408 {};
#else
class DisplayActivity : public Activity, public WriteStream<DisplayActivity> {
public:
    constexpr static bool OK = false;
    DisplayActivity& Display;

    DisplayActivity()
        : Display(*this)
    {}
    
    void Setup() {}

    void OnLoop(const ActivityContext&) {}

    void send(const AW::StringBuf&) {}
};
#endif

