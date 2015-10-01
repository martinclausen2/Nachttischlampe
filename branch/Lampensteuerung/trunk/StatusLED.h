/*
Controls Status LED connected to OCA (blue), OCB (green), OCC (red)
*/

#ifndef __StatusLED_H_GUARD
#define __StatusLED_H_GUARD 1

unsigned char LEDFlashCount;	//Number of flashes currently required, should not exceed 0x7F ...
unsigned char LEDFlashSeqCounter;	//Number of flashes * 2 (on & off) to be produced in this sequence
unsigned char LEDLimitFlashTimer;	//Software timer to decouple flashing frequency from calling frequency
__bit LEDTempOn;			//Flag to indicate active temperature control, required for LED control

#define LEDFlashMaxSeq	8	//max. number of flashes in a squence, should not exceed 0x7F ...
#define LEDmaxFlashTimer	6	//cycles to execute before next flash toggel happens
#define LEDmaxLimitFlashTimer 4	//cycles to execute before LED status is restored

#define BlueLEDPort 	P2_6
#define GreenLEDPort 	P1_6
#define RedLEDPort 	P1_7

__code unsigned char colorTable[9][3] = { { 0x00, 0x00, 0x00},

					{ 0x7F, 0x7F, 0x7F},

					{ 0x00, 0x00, 0xFF},
					{ 0xFF, 0x00, 0x00},
					{ 0x00, 0xFF, 0x00},

					{ 0x5F, 0x00, 0xBF},
					{ 0x5F, 0x7F, 0x00},
					{ 0x5F, 0x3F, 0x9F},
					{ 0x5F, 0x8F, 0x4F},
			 	        };

void LEDOff();

void LEDStandby();

void LEDOn();

void LEDOptions();

void LEDOptionsFlash(unsigned char i);

void LEDCancel();

void LEDOption(unsigned char i);

void LEDOvertemp();

void LEDLimit();

void LEDSetupLimit();

void LEDFlashing();

void LEDFlashReset();

void LEDSetColor(unsigned char i);

#endif