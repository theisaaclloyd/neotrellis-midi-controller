/* === definitions ================================== */

#define INT_PIN 5

#define jump 2

#define WHITE 0xffffff	   //0x009f60
#define WHITE_DIM 0x191919 //0x000503

#define RED 0xff0000
#define RED_DIM 0x190000

#define GREEN 0x00ff00
#define GREEN_DIM 0x001900

#define BLUE 0x0000ff
#define BLUE_DIM 0x000019

#define OFF 0x000000

//0x009f60 - light ice blue
//0x00cf30 - light green

#define DIM 0
#define BRIGHT 1

#define CHROMATIC -1
#define MAJOR 0
#define MINOR 1

#define ROWS 2
#define COLS 2

#define channel 0

/* === trellis ====================================== */
Adafruit_NeoTrellis t_array[ROWS][COLS] = {
	{Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x2E)},
	{Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x30)}
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, ROWS, COLS);

/* === variables ==================================== */
int color;

int ccNum;

int ccVals[4] = {64, 64, 64, 64};

int offsets[2][7] = {
	{0, 2, 4, 5, 7, 9, 11}, // Major
	{0, 2, 3, 5, 7, 8, 10}	// Minor
};

int notes[8][7];

int ccColors[2][8]{
	{BLUE_DIM, GREEN_DIM, BLUE_DIM, GREEN_DIM, BLUE_DIM, GREEN_DIM, BLUE_DIM, GREEN_DIM}, // Dim
	{BLUE, GREEN, BLUE, GREEN, BLUE, GREEN, BLUE, GREEN}								  // Bright
};

/* === functions ==================================== */

void noteOn(byte pitch, byte velocity)
{
	midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
	MidiUSB.sendMIDI(noteOn);
	MidiUSB.flush();
}

void noteOff(byte pitch)
{
	midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, 0};
	MidiUSB.sendMIDI(noteOff);
	MidiUSB.flush();
}

void controlChange(byte control, byte value)
{
	midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
	MidiUSB.sendMIDI(event);
	MidiUSB.flush();
}

void makeScale(int root, int type)
{
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 7; x++)
		{
			if (type == CHROMATIC) notes[y][x] = root++;
			else notes[y][x] = root + offsets[type][x] + y * 12;
			delay(1);
		}
	}
}

int getColor(int key, int brightness)
{
	if (brightness == SEESAW_KEYPAD_EDGE_RISING) color = ccColors[BRIGHT][key];
	else if (brightness == SEESAW_KEYPAD_EDGE_FALLING) color = ccColors[DIM][key];
	else color = ccColors[brightness][key];
	return color;
}

int handleMidiOnOff(byte onoff, byte number)
{
	int keyNum;
	int xVal;
	int yVal;

	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 7; x++)
		{
			if (notes[y][x] != number) continue;
			else if (notes[y][x] == number)
			{
				keyNum = 1 + x + y + 6 * y;
				xVal = x;
				yVal = y;
				break;
			}
		}
	}

	if (onoff == 9)	color = RED;
	else if (onoff == 8)
	{
		if (xVal < 7) color = WHITE_DIM;
		else if (xVal == 7) color = BLUE_DIM;
	}

	trellis.setPixelColor(xVal, yVal, color);
	trellis.show();

	return keyNum;
}

int handleCC(int num, int val)
{
	int keyNum;
	if (num >= 0 && num <= 4)
	{
		if (val > ccVals[num]) //UP (keynum = even)
		{
			ccVals[num] = val;
			keyNum = num * 2;
			trellis.setPixelColor(7, keyNum, RED);
		}
		else if (val < ccVals[num]) // DOWN (keynum = odd)
		{
			ccVals[num] = val;
			keyNum = (num * 2) + 1;
			trellis.setPixelColor(7, keyNum, RED);
		}
		else if (val == ccVals[num]) // BOTH (keynum = even+odd)
		{
			ccVals[num] = val;
			trellis.setPixelColor(7, num * 2, RED_DIM);
			trellis.setPixelColor(7, (num * 2) + 1, RED_DIM);
		}
		return keyNum;
	}
}

void readMidi()
{
	midiEventPacket_t rx;
	rx = MidiUSB.read();
	if (rx.header == 0x09 || rx.header == 0x08)
	{ // On/Off
		int keynum = handleMidiOnOff(rx.header, rx.byte2);
		Serial.print("Received: ");
		Serial.print(rx.header == 9 ? "Note On" : (rx.header == 8 ? "Note Off" : "Other"));
		Serial.print(": ");
		Serial.print(rx.byte2);
		Serial.print(" velocity: ");
		Serial.print(rx.byte3);
		Serial.printf(" (Key %i)\n", keynum);
	}
	else if (rx.header == 0x0B)
	{ //CC
		int keynum = handleCC(rx.byte2, rx.byte3);
		Serial.print("Received: ");
		Serial.print(rx.header == 0x0B ? "CC" : "Other");
		Serial.print(": ");
		Serial.print(rx.byte2);
		Serial.print(" value: ");
		Serial.print(rx.byte3);
		Serial.printf(" (Key %i)\n", keynum);
	}
	else return;
}

TrellisCallback blink(keyEvent evt)
{
	int x = evt.bit.NUM % (4 * COLS);
	int y = evt.bit.NUM / (4 * COLS);

	if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
	{
		if (x < 7)
		{
			Serial.printf("Sending Note On: %i (Key %i)\n", notes[y][x], 1 + x + y + 6 * y);
			noteOn(notes[y][x], 100);
			color = WHITE;
		}

		else if (x == 7)
		{
			ccNum = (y / 2);
			if (y % 2)
			{
				ccVals[ccNum] = ccVals[ccNum] - jump;
				if (ccVals[ccNum - 1] < 0) ccVals[ccNum] = 0;
				else controlChange(ccNum, ccVals[ccNum]);
				Serial.printf("Sending CC %i- (%i)\n", ccNum, ccVals[ccNum]);
			}

			else
			{
				ccVals[ccNum] = ccVals[ccNum] + jump;
				if (ccVals[ccNum] > 127) ccVals[ccNum] = 127;
				else controlChange(ccNum, ccVals[ccNum]);
				Serial.printf("Sending CC %i+ (%i)\n", ccNum, ccVals[ccNum]);
			}
			color = getColor(y, evt.bit.EDGE);
		}
		trellis.setPixelColor(x, y, color);
	}

	else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
	{
		if (x < 7)
		{ // Regular Note
			Serial.printf("Sending Note Off: %i (Key %i)\n", notes[y][x], 1 + x + y + 6 * y);
			noteOff(notes[y][x]);
			color = WHITE_DIM;
		}

		else if (x == 7) color = getColor(y, evt.bit.EDGE);

		trellis.setPixelColor(evt.bit.NUM, color);
	}

	trellis.show();
	return 0;
}

uint32_t Wheel(byte WheelPos)
{
	if (WheelPos < 85)
	{
		return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	}
	else if (WheelPos < 170)
	{
		WheelPos -= 85;
		return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	else
	{
		WheelPos -= 170;
		return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	return 0;
}

void readTrellis()
{
	if (!digitalRead(INT_PIN))
	{
		Serial.println("int pin low");
		trellis.read();
	}
}

#define A0 21
#define A_0 22
#define B0 23
#define C1 24
#define C_1 25
#define D1 26
#define D_1 27
#define E1 28
#define F1 29
#define F_1 30
#define G1 31
#define G_1 32
#define A1 33
#define A_1 34
#define B1 35
#define C2 36
#define C_2 37
#define D2 38
#define D_2 39
#define E2 40
#define F2 41
#define F_2 42
#define G2 43
#define G_2 44
#define A2 45
#define A_2 46
#define B2 47
#define C3 48
#define C_3 49
#define D3 50
#define D_3 51
#define E3 52
#define F3 53
#define F_3 54
#define G3 55
#define G_3 56
#define A3 57
#define A_3 58
#define B3 59
#define C4 60
#define C_4 61
#define D4 62
#define D_4 63
#define E4 64
#define F4 65
#define F_4 66
#define G4 67
#define G_4 68
#define A4 69
#define A_4 70
#define B4 71
#define C5 72
#define C_5 73
#define D5 74
#define D_5 75
#define E5 76
#define F5 77
#define F_5 78
#define G5 79
#define G_5 80
#define A5 81
#define A_5 82
#define B5 83
#define C6 84
#define C_6 85
#define D6 86
#define D_6 87
#define E6 88
#define F6 89
#define F_6 90
#define G6 91
#define G_6 92
#define A6 93
#define A_6 94
#define B6 95
#define C7 96
#define C_7 97
#define D7 98
#define D_7 99
#define E7 100
#define F7 101
#define F_7 102
#define G7 103
#define G_7 104
#define A7 105
#define A_7 106
#define B7 107
#define C8 108
