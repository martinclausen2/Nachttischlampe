/**
 * Main Program of LED Leuchte, a program to control two LED chains
 * @file Main.c
 * use a P89LPC936 with 6Mhz resonator
 * Compiler options: -mmcs51 --iram-size 256 --xram-size 512 --code-size 16368 --std-sdcc89 --model-medium, all optimastions on
 * SDCC 3.2.0
 * 
 * 
 * V2 2013-05-06
 * fix: alarmgflag was not switched off, if device was swithced off by remote signal
 * V1 2013-03-25
 * added option to skip alarms by turning knob in standby
 */

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>
#include <stdio.h>

#define KeyPressShort	8
#define KeyPressLong	32

#define isrregisterbank	2		//for all isr on the SAME priority level 

void T0_isr(void) __interrupt (1);		//int from Timer 0 to read RC5 state
void I2C_RTC_isr(void) __interrupt (2);	//int from clk every minute to update the time, sets flag to be processed by main
void T1_isr(void) __interrupt (3);		//int from Timer 1 to read encoder state
void WDT_RTC_isr(void) __interrupt (10);	//int from internal RTC to update PWM read keys

unsigned char Minutes2Signal;
unsigned char DisplayDimCnt;

__bit volatile RefreshTimeRTC;		// next minute, get time from rtc and display it
__bit volatile Flag40ms;
__bit RefreshTime;			// get time from rtc and display it
__bit Alarmflag;
__bit LightOn;
__bit FocusBacklight;

#include "Options.h"
#include "LCD_EA_DOG_SPI.H"

#include "InitMCU.c"
#include "LCD_EA_DOG_SPI.c"
#include "Encoder.c"
#include "Keys.c"
#include "GetBrightness.c"
#include "SetBrightness.c"
#include "I2C_RTC.c"
#include "RC5.c"
#include "Options.c"
#include "AcusticAlarm.c"

/** Main loop */
void main()
{
	InitMCU();

	LCD_Init3V();
	LCD_SetContrast(Read_EEPROM(EEAddr_LCDContrast));
	LCD_SendString("Start");

	SetupRTC();
	RefreshTime=1;

	//load Ram values from EEPROM
	Brightness_start[0]=Read_EEPROM(EEAddr_DispBrightness);
	ReceiverMode=Read_EEPROM(EEAddr_ReceiverMode);
	SenderMode=Read_EEPROM(EEAddr_SenderMode);
	RC5Addr=Read_EEPROM(EEAddr_RC5Addr);

	// Infinite loop
	while(1) {

		if (Flag40ms)
			{
			PWM_StepDim();			// do next dimming step
			Alarm_StepDim_all();
			if (1 < DisplayDimCnt)		// LCD backlight fadout count down
				{
				--DisplayDimCnt;
				}
			else if (1 == DisplayDimCnt)	// switch display backlight off?
				{
				SwLightOff(0);
				DisplayDimCnt=0;
				skipAlarmCnt = (skipAlarmCnt & skipAlarmMask) | skipAlarmhalfStep;	//set lsb, so we can use the encode to switch on again without changing alarm settings
				}
			else				// do not measure intensity with backlight on
				{
				MeasureExtBrightness();
				}
			Flag40ms=0;
			}

		if (RefreshTime || RefreshTimeRTC)
			{
			GetTimeDateRTC();
			LCD_SendTime();
			if (RefreshTimeRTC)			//execute only once(!) every minute or we can not leave the alarm during its first minute, 
				{
				CheckAlarm();
				}
			if (0==LightOn)
				{
				LCD_NextAlarm();
				}
			if (1 < Minutes2Signal)
				{
				LCD_SnoozeTime();
				if (RefreshTimeRTC)
					{
					--Minutes2Signal;
					}
				}
			else if (1 == Minutes2Signal)
				{
				LCD_SendCmd(LCDSet2ndLine);
				printf_fast("Alarm!          ");
				AcusticDDSAlarm();
				Minutes2Signal=Read_EEPROM(EEAddr_AlarmTimeSnooze);	//start snooze
				}
			RefreshTime=0;
			RefreshTimeRTC=0;
			}

		if (12==rCounter)
			{
			DecodeRemote();
			}

		// A Key was pressed if OldKeyState != 0 and Keystate = 0
		// OldKeyState = 0 must be set by receiving program after decoding as a flag
		if ((KeySelect == OldKeyState) && (0 == KeyState))
			{
			OldKeyState=0;				//Ack any key anyway
			SwBackLightOn(fadetime);
			if (Minutes2Signal)
				{
				SwBackLightOn(1);			//Switch LCD backlight instantly on
				AlarmSnoozeEnd();
				}
			else if (KeyPressShort > KeyPressDuration)
				{
				if (LightOn) 
					{
					FocusBacklight=!FocusBacklight;
					LCD_SendBrightness(FocusBacklight+1);
					}
				else
					{
					SendRC5(RC5Addr_com, RC5Cmd_On, 1, ComModeAll, RC5Cmd_Repeats);
					SwAllLightOn();
					}
				}
			else if (KeyPressLong > KeyPressDuration)
				if (LightOn)
					{
					SendRC5(RC5Addr_com, RC5Cmd_Off, 1, ComModeAll, RC5Cmd_Repeats);
					SwAllLightOff();
					}
				else
					{
					if (ComModeConditional<=SenderMode)			//reset to eeprom value in swalllightoff()
						{
						SenderMode=ComModeAll;
						}
					SendRC5(RC5Addr_com, RC5Cmd_On, 1, ComModeAll, RC5Cmd_Repeats);
					SwAllLightOn();
					}
			else
				{
				Alarmflag=0;
				SwBackLightOn(1);			//Switch LCD backlight instantly on
				Options();
				if (LightOn)
					{
					LCD_SendBrightness(FocusBacklight+1);
					}
				}
			}

		// A Rotation occured if EncoderSteps!= 0
		// EncoderSteps = 0 must be set by receiving program after decoding
		if (EncoderSteps)
			{
			Alarmflag=0;
			SwBackLightOn(fadetime);
			if (LightOn)
				{
				PWM_SetupDim(FocusBacklight+1, Brightness_steps, EncoderSteps);
				EncoderSteps = 0;								//ack any steps
				LCD_SendBrightness(FocusBacklight+1);
				SendRC5(RC5Addr_front+FocusBacklight, (Brightness[FocusBacklight+1]>>1) & 0x3F, Brightness[FocusBacklight+1] & 0x01, ComModeAll, RC5Value_Repeats);
				}
			else 
				{
				EncoderSetupValue(&skipAlarmCnt, maxskipAlarmCnt, 0);
				RefreshTime=1;
				}
			}
		PCON=MCUIdle;				//go idel, wake up by any int
	}
}