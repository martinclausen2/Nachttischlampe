/**
 * Main Program of LampenSteuerung, a program to control a LED power supply by PWM or analogue signal
 * @file Main.c
 * use a P89LPC936 with internal 7.373 MHz RC oszilator
 * use a P89LPC935 or 936 with internal 7.373 MHz RC oszilator for the version without a LCD
 * Compiler options: -mmcs51 --iram-size 256 --xram-size 512 --code-size 16368 --std-sdcc89 --model-medium, all optimations on
 * SDCC 3.4.0
 */

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>
#include <stdio.h>

//#define LCD

#define KeyPressShort	20
#define KeyPressLong	60
#define KeyPressLonger	KeyPressLong*2

#define isrregisterbank	2		//for all isr on the SAME priority level 

void T0_isr(void) __interrupt (1);		//int from Timer 0 to read RC5 state
void T1_isr(void) __interrupt (3);		//int from Timer 1 to read encoder state
void WDT_RTC_isr(void) __interrupt (10);	//int from internal RTC to update PWM, read keys, ...

__bit volatile TimerFlag;
__bit Alarmflag;
__bit LightOn;
__bit enableExtBrightness;

#include "Options.h"
#ifdef LCD
#include "LCD_EA_DOG_SPI.h"
#endif
#include "StatusLED.h"
#include "GetTemperature.h"

#include "EEPROM.c"
#include "InitMCU.c"
#ifdef LCD
#include "LCD_EA_DOG_SPI.c"
#endif
#include "Encoder.c"
#include "Keys.c"
#include "GetBrightness.c"
#include "SetBrightness.c"
#include "GetTemperature.c"
#include "MotionDetector.c"
#include "RC5.c"
#include "Options.c"
#include "StatusLED.c"

/** Main loop */
void main()
{
	unsigned char actionCounter=0xFF;
	InitMCU();

	#ifdef LCD
	LCD_Init3V();
	LCD_SetContrast(Read_EEPROM(EEAddr_LCDContrast));
	LCD_SendString2ndLine("LCD ok");
	#endif

	//load RAM values from EEPROM
	ReceiverMode=Read_EEPROM(EEAddr_ReceiverMode);
	RC5Addr=Read_EEPROM(EEAddr_RC5Addr);

	InitBrightness();

	SwLightOn();

	// Infinite loop
	while(1) {
		if (TimerFlag)
			{
			TimerFlag=0;
			++actionCounter;
			PWM_StepDim();		// do next dimming step
			switch (LimitOutput())	// decide if temperature is ok and what to do about it, LimitOutput will act on PWM values
				{
				case 1:	//derating temperature
					if (LightOn)
						{
						LEDTempDerating();	// flash status led red / white
						}
					break;
				case 2:	//overtemprature
					if (LightOn)
						{
						SwLightOff();
						#ifdef LCD
						LCD_SendStringFill2ndLine("Temperature!");
						#endif
						}
					break;
				default: //normal operation
					LEDTempReset();
					break;
				}
			PWM_Set();

			// check keys here since we can have only new input if timerflag was set by the keys interrupt program
			// Select key is pressed, show preview of action
			// need to check each time to generate single events directly from KeyPressDuration counter
			if (KeySelect == KeyState)
				{
				if (KeyPressShort == KeyPressDuration)
					{
					#ifdef LCD
					LCD_SendStringFill2ndLine("Enter Options");
					#endif
					LEDOptions();
					}
				else if (KeyPressLong == KeyPressDuration)
					{
					#ifdef LCD
					LCD_SendStringFill2ndLine(&Canceltext[0]);
					#endif
					LEDCancel();
					}
				}

			switch (actionCounter)
				{
				case 0:
					DecodeRemote();
					MotionDetector();

					MeasureTemperature();		// always measure temp or we can not exit overtemp anymore
					#ifdef LCD
					LCD_Temperature();
					#endif
					break;
				case 1:
					if (LightOn)
						{
						StoreBrightness();	// store brightness if reuqired
						Alarm_StepDim();		// do next alarm dim step if required
						}
					else
						{
						if(!PWM_setlimited)	//switch ADC only on if DAC is really not needed any more
							{
							ADMODB = ADC1;
							DAC1Port = 0;
							MeasureExtBrightness();
							}
						WriteTimer=0;
			 			if(overTemp)
			 				{
							LEDOverTemp();	// flash status led red
							}
						}
					break;
				case 2:
					// A Key was pressed if OldKeyState != 0 and Keystate = 0
					// OldKeyState = 0 must be set by receiving program after decoding as a flag
					if ((KeySelect == OldKeyState) && (0 == KeyState))
						{
						OldKeyState=0;		//Ack any key anyway
						MotionDetectorTimer=0;	//reset any Motion Detector activity
						if (KeyPressShort > KeyPressDuration)
							{
							if (LightOn)
								{
								SwLightOff();
								}
							else
								{
								SwLightOn();
								}
							}
						else 
							{
							if (KeyPressLong > KeyPressDuration)
								{
								Alarmflag=0;
								Options();
								}
							if (LightOn)	// Cancel key pressing or return from options, refresh display
								{
								LCD_SendBrightness();
								LEDOn();
								}
							else
								{
								#ifdef LCD
								LCD_SendStringFill2ndLine("Standby");
								#endif
								LEDStandby();
								}
							}
						}
					break;
				case 3:
					// A Rotation occured if EncoderSteps!= 0
					// EncoderSteps = 0 must be set by receiving program after decoding
					if (EncoderSteps)
						{
						Alarmflag=0;
						if (LightOn)
							{
							PWM_SetupDim(Brightness_steps, EncoderSteps);
							EncoderSteps = 0;								//ack any steps
							LCD_SendBrightness();
							WriteTimer=WriteTime;
							}
						}

					actionCounter=0xFF;	//last time slot, do reset counter with increment to 0
					break;
				}
			}
		PCON=MCUIdle;				//go idel, wake up by any int
	}
}