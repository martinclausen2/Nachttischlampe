/**
 * Main Program of LED Leuchte, a program to control two LED chains
 * @file Main.c
 * use a P89LPC936 with 6Mhz resonator
 * Compiler options: -mmcs51 --iram-size 256 --xram-size 512 --code-size 16368 --std-sdcc89 --model-medium, all optimastions on
 * SDCC 3.3.0
 */

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>
#include <stdio.h>

			// set to 1 for ZXLD1374 since the chip will not switch on at PWM frequencies lower than 100Hz (SetBrightness.C)
			// this will also activate push-pull outputs (InitMCU.C)
#define HighPWM	1	// set to 0 for ZXLD1362 to obtain a wider dimming range at the cost of a lower PWM frequency / flickering

#define KeyPressShort	20
#define KeyPressLong	60
#define KeyPressLonger	KeyPressLong*2

#define isrregisterbank	2		//for all isr on the SAME priority level 

void T0_isr(void) __interrupt (1);		//int from Timer 0 to read RC5 state
void I2C_RTC_isr(void) __interrupt (2);	//int from clk every minute to update the time, sets flag to be processed by main
void T1_isr(void) __interrupt (3);		//int from Timer 1 to read encoder state
void WDT_RTC_isr(void) __interrupt (10);	//int from internal RTC to update PWM read keys

unsigned char Minutes2Signal;
unsigned char DisplayDimCnt;

__bit volatile RefreshTimeRTC;		// next minute, get time from rtc and display it
__bit volatile TimerFlag;
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
#include "AcusticAlarm.c"
#include "Options.c"

/** Main loop */
void main()
{
	InitMCU();

	LCD_Init3V();
	LCD_SetContrast(Read_EEPROM(EEAddr_LCDContrast));
	LCD_SendString("LCD ok ");

	SetupRTC();
	RefreshTime=1;
	LCD_SendString("RTC ok ");

	//load RAM values from EEPROM
	Brightness_start[0]=Read_EEPROM(EEAddr_DispBrightness);
	ReceiverMode=Read_EEPROM(EEAddr_ReceiverMode);
	SenderMode=Read_EEPROM(EEAddr_SenderMode);
	RC5Addr=Read_EEPROM(EEAddr_RC5Addr);
	Update_PWM_Offset(1);
	Update_PWM_Offset(2);

	// Infinite loop
	while(1) {
		if (TimerFlag)
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
				//set lsb, so we can use the encode to switch on again without changing alarm settings
				skipAlarmCnt = (skipAlarmCnt & skipAlarmMask) | skipAlarmhalfStep;
				}
			else				// do not measure intensity with backlight on
				{
				MeasureExtBrightness();
				}
			TimerFlag=0;

			// check keys here since we can have only new input if timerflag was set by the keys interrupt program
			// Select key is pressed, show preview of action
			if (KeySelect == KeyState)
				{
				SwBackLightOn(fadetime);	//switch on or stay on
				if (Minutes2Signal)
					{
					if(0 == KeyPressDuration)
						{
						LCD_SendStringFill2ndLine("Set Snooze End");
						}
					else if (KeyPressLong == KeyPressDuration)
						{
						LCD_SendStringFill2ndLine("End Alarm");
						Beep();
						}
					}
				else if (KeyPressShort == KeyPressDuration)
					if (LightOn)
						{
						LCD_SendStringFill2ndLine("Enter Standby");
						Beep();
						}
					else
						{
						LCD_SendStringFill2ndLine("Switch All On");
						Beep();
						}
				else if (KeyPressLong == KeyPressDuration)
					{
					LCD_SendStringFill2ndLine("Enter Options");
					Beep();
					}
				else if (KeyPressLonger == KeyPressDuration)
					{
					LCD_SendStringFill2ndLine(&Canceltext[0]);
					Beep();
					}
				}

			// A Key was pressed if OldKeyState != 0 and Keystate = 0
			// OldKeyState = 0 must be set by receiving program after decoding as a flag
			else if ((KeySelect == OldKeyState) && (0 == KeyState))
				{
				OldKeyState=0;		//Ack any key anyway
				if (Minutes2Signal)
					{
					if (KeyPressLong > KeyPressDuration)
						{
						AlarmSnoozeEnd();
						}
					else
						{
						AlarmEnd();
						LCD_SendBrightness(FocusBacklight+1);
						}
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
					{
					if (LightOn)
						{
						SendRC5(RC5Addr_com, RC5Cmd_Off, 1, ComModeAll, RC5Cmd_Repeats);
						SwAllLightOff();
						}
					else
						{
						if (ComModeConditional<=SenderMode)		//reset to eeprom value in swalllightoff()
							{
							SenderMode=ComModeAll;
							}
						SendRC5(RC5Addr_com, RC5Cmd_On, 1, ComModeAll, RC5Cmd_Repeats);
						SwAllLightOn();
						}
					}
				else if (KeyPressLonger > KeyPressDuration)
					{
					Alarmflag=0;
					Options();
					}
				else
					{
					if (LightOn)	// Cancel key pressing, refresh display
						{
						LCD_SendBrightness(FocusBacklight+1);
						}
					RefreshTime=1;
					}
				}
			}

		if (RefreshTime || RefreshTimeRTC)
			{
			GetTimeDateRTC();
			LCD_SendTime();
			if (RefreshTimeRTC)		//execute only once(!) every minute or we can not leave the alarm during its first minute
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
				SwBackLightOn(1);					//switch on now
				LCD_SendStringFill2ndLine(&Alarmtext[0]);
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