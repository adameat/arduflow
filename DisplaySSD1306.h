#include <ArduinoWorkflow.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

namespace AW {

class DisplaySSD1306 : public TActor {
public:
	void SetContrast(uint8_t contrast) {
		if (DisplayFound) {
			Display.setContrast(contrast);
		}
	}

	void DisplayOff() {
		if (DisplayFound) {
			Display.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
		}
	}

	void DisplayOn() {
		if (DisplayFound) {
			Display.ssd1306WriteCmd(SSD1306_DISPLAYON);
		}
	}

protected:
	SSD1306AsciiWire Display;
	bool DisplayFound = false;

	void OnEvent(TEventPtr event, const TActorContext& context) override {
		switch (event->EventID) {
		case TEventBootstrap::EventID:
			return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
		case TEventSerialData::EventID:
			return OnSerialData(static_cast<TEventSerialData*>(event.Release()), context);
		}
	}

	void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext&/* context*/) {
		static const uint8_t address = 0x3c;
		TWire::BeginTransmission(address);
		if (TWire::EndTransmission()) {
			DisplayFound = true;
			Display.begin(&Adafruit128x64, address);
			Display.setFont(Adafruit5x7);
			Display.setScroll(true);
			Display.clear();
		}
	}

	void OnSerialData(TUniquePtr<TEventSerialData> event, const TActorContext&/* context*/) {
		if (DisplayFound) {
			for (unsigned int i = 0; i < event->Data.size(); ++i) {
				Display.write(event->Data[i]);
			}
			Display.println();
		}
	}
};

}
