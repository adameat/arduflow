#include <pin_magic.h>
#include <registers.h>
#include <SPFD5408_Adafruit_GFX.h>
#include <SPFD5408_Adafruit_TFTLCD.h>
#include <SPFD5408_TouchScreen.h>

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

class DisplayActivitySPFD5408 : public Activity, public WriteStream<DisplayActivitySPFD5408> {
public:
    DisplayActivitySPFD5408& Display;
    constexpr static bool OK = true;
    
    DisplayActivitySPFD5408()
        : Display(*this)
        , TFT(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET)
        , LineHeight(8)
        , Cursor(false)
    {}
    
    void Setup() {
        TFT.reset();
        uint16_t identifier = 0x9341; //tft.readID();
        TFT.begin(identifier);
        TFT.setRotation(0);
        TFT.fillScreen(BLACK);
        TFT.setTextColor(GREEN);
        TFT.setTextSize(1);
    }

    void OnLoop(const ActivityContext& context) {
        if (CursorTrigger.IsTriggered(context)) {
            CursorBlink();
        }
    }

    void ScrollUp(int amount = 1) {
        int width = TFT.width();
        int max_height = TFT.height() - amount;
        for (int y = 0; y < max_height; ++y)
            for (int x = 0; x < width; ++x) {
                auto pix = TFT.readPixel(x, y + amount);
                TFT.drawPixel(x, y, pix);
            }
        TFT.fillRect(0, max_height, TFT.width(), TFT.height() - 1, BLACK);
    }

    void CursorOff() {
        if (Cursor) {
            TFT.fillRect(TFT.getCursorX(), TFT.getCursorY(), 4, 8, BLACK);
            Cursor = false;
        }
    }

    void CursorOn() {
        if (!Cursor) {
            TFT.fillRect(TFT.getCursorX(), TFT.getCursorY(), 4, 8, GREEN);
            Cursor = true;
        }
    }

    void CursorBlink() {
        if (Cursor) {
            CursorOff();
        } else {
            CursorOn();
        }
    }

    void send(const StringBuf& str) {
        CursorOff();
        int cursY1 = TFT.getCursorY();
        ((Print&)TFT).write(str.data(), str.size());
        int cursY2 = TFT.getCursorY();
        if (cursY2 >= TFT.height()) {
            TFT.setCursor(0, 0);
            cursY2 = 0;
        }
        if (cursY2 != cursY1) {
            TFT.fillRect(0, cursY2, TFT.width(), LineHeight, BLACK);
        }
        CursorOn();
    }

protected:
    PeriodicTrigger<200> CursorTrigger;
    Adafruit_TFTLCD TFT;
    int LineHeight;
    bool Cursor;
};

