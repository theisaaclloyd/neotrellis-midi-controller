#include "Adafruit_NeoTrellis.h"
#include "MIDIUSB.h"
#include "stuff.h"

#define SCALE MAJOR //MAJOR, MINOR, or CHROMATIC
#define ROOT C1		//Lowest note

void setup()
{
	Serial.begin(115200);
	//while (!Serial);
	Serial.println("Hello");
	pinMode(INT_PIN, INPUT_PULLUP);

	if (!trellis.begin())
	{
		Serial.println("failed to begin trellis");
		while (1);
	}

	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			trellis.setPixelColor(y, x, Wheel(map(x * y, 0, 64, 0, 255)));
			trellis.show();
			delay(30);
		}
	}

	makeScale(ROOT, SCALE);

	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
			trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
			trellis.registerCallback(x, y, blink);

			if (x < 7) color = WHITE_DIM;
			else if (x == 7) color = getColor(y, DIM);
			
			trellis.setPixelColor(x, y, color);
			trellis.show();
			delay(10);
		}
	}
}

void loop()
{
	readTrellis();
	readMidi();
}
