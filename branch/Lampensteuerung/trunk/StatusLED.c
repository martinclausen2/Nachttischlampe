/*
Controls Status LED connected to OCA (blue), OCB (green), OCC (red)
*/

#include "StatusLED.h"

void LEDOff()
{
	LEDSetColor(0);
	LEDFlashCount = 0;
}

void LEDStandby()
{
	LEDSetColor(2);
	LEDFlashCount = 0;
}

void LEDOn()
{
	LEDSetColor(1);
	LEDFlashCount = 0;
}

void LEDOptions()
{
	LEDSetColor(3);
	LEDFlashCount = 0;
}

void LEDCancel()
{
	LEDSetColor(4);
	LEDFlashCount = 0;
}

void LEDOptionsFlash(unsigned char i)	//i should not exceed LEDmax Flash
{
	LEDSetColor(3);		//add offset reserved for Standby and ON
	LEDFlashCount = i*2+1;
}

void LEDOption(unsigned char i)	//i should not exceed colorTable length - 5
{
	LEDSetColor(i+5);		//add offset reserved for other status
}

void LEDOverTemp()		//flashes red LED to indicate overtemperature
{
	unsigned char static LEDFlashTimer;	//Software timer to decouple flashing frequency from calling frequency
	__bit static Status;
	if (LEDFlashTimer)
		{
		--LEDFlashTimer;
		}
	else
		{
		LEDFlashTimer = LEDmaxFlashTimer;
		Status = !Status;
		RedLEDPort = Status;
		}
}

void LEDTempDerating()		//flashes green & blue LED to indicate temperature derating
				//flashes will appear as red / white if the basic color is white
{
	unsigned char static LEDFlashTimer;	//Software timer to decouple flashing frequency from calling frequency
	__bit static Status;
	if (LEDFlashTimer)
		{
		--LEDFlashTimer;
		}
	else
		{
		LEDFlashTimer = LEDmaxFlashTimer;
		Status = !Status;
		GreenLEDPort = Status;
		BlueLEDPort = Status;
		}
}

void LEDTempReset()
{
	RedLEDPort = 1;
	GreenLEDPort = 1;
	BlueLEDPort = 1;
}

void LEDFlashing()		//flashes the blue LED to indicate current option
{
	unsigned char static LEDFlashTimer;	//Software timer to decouple flashing frequency from calling frequency
	if (LEDFlashTimer)
		{
		--LEDFlashTimer;
		}
	else
		{
		LEDFlashTimer = LEDmaxFlashTimer;
		if (LEDFlashSeqCounter <= (LEDFlashCount))	//toggel pin
			{
				BlueLEDPort = LEDFlashSeqCounter & 0b01;
			}
		if (LEDFlashSeqCounter)
			{
			--LEDFlashSeqCounter;
			}
		else
			{
			LEDFlashSeqCounter = LEDFlashMaxSeq << 1 & 0xFE;
			}
		}
}

void LEDFlashReset()
{
	LEDFlashSeqCounter = 0;
	BlueLEDPort = 1;
}

void LEDSetColor(unsigned char i)
{
	unsigned int tempPWM;
	tempPWM = colorTable[i][0];
	tempPWM *= tempPWM;

	OCRAL = tempPWM & 0x00FF;
	OCRAH = (tempPWM >> 8) & 0x00FF;

	tempPWM = colorTable[i][1];
	tempPWM *= tempPWM;

	OCRBL = tempPWM & 0x00FF;
	OCRBH = (tempPWM >> 8) & 0x00FF;

	tempPWM = colorTable[i][2];
	tempPWM *= tempPWM;

	OCRCL = tempPWM & 0x00FF;
	OCRCH = (tempPWM >> 8) & 0x00FF;

	TCR21 = PLLSetting;	//Set PLL prescaler and start CCU register update
}
