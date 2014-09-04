/**
 * Options of LampenSteuerung incl. EEPROM access and passive Alarm
 * @file Options.c
 */

#include <p89lpc935_6.h>
#include "Options.h"

unsigned char Read_EEPROM(unsigned char address)
{
	DEECON=0;		//clear EEIF, Byte read / write, first page
	DEEADR=address;
	while( 0 == (DEECON & EEIF) ){}
	return DEEDAT;
}

void Write_EEPROM(unsigned char address, unsigned char data2write)
{
	if (Read_EEPROM(address)!=data2write)	//write only if content has changed!
		{
		DEECON=0;	//clear EEIF, Byte read / write, first page
		DEEDAT=data2write;
		DEEADR=address;
		while( 0 == (DEECON & EEIF) ){}
		}
}

//wake-up light dimming
void Alarm_StepDim()
{
	if (Alarmflag)
		{
		if (AlarmDim_Cnt)
			{
			--AlarmDim_Cnt;				//count down step
			}
		else							//dimming step
			{
			if (Brightness < Read_EEPROM(EEAddr_AlarmFrontBrightness))
				{
				AlarmDim_Cnt=AlarmDim_Cnt_Reload;	//reload countdown
				PWM_SetupDim(Brightness_steps, 1);	//setup brightness
				}
			else
				{
				Alarmflag=0;				//we reached targetbrightness!
				}
			}
		}
}

//prepare wake-up light
void SetupAlarmDim()
{
	AlarmDim_Cnt_Reload=(Read_EEPROM(EEAddr_LightFading)*25*60)/Read_EEPROM(EEAddr_AlarmFrontBrightness);
	AlarmDim_Cnt=AlarmDim_Cnt_Reload;
}


//execute an alarm
void Alarm()
{
	if (0==Alarmflag)
		{
		SetupAlarmDim();		// fade in to required brightness in on or off,
					// no fade in if already brighter, Alarm_StepDim() takes care of this behavior
		LightOn=1;
		Alarmflag=1;
		}
}

void AlarmEnd()
{
}

void LCD_MinuteOff(unsigned char Value, unsigned char maxValue)
{
	LCD_SendCmd(LCDSet2ndLine);
	if (0==Value)			//this value should not be used!
		{
		printf_fast("off       ");
		LEDOff();
		}
	else if (1==Value)
		{
		printf_fast("instantly ");
		LEDOption(0);
		}
	else if (2==Value)
		{
		printf_fast(" 1 Minute ");
		LEDOption(1);
		}
	else 
		{
		printf_fast("%2d Minutes", Value-1);
		if (Value < (maxValue>>2 & 0x3F))
			{
			LEDOption(1);
			}
		else if (Value < (maxValue>>1 & 0x7F))
			{
			LEDOption(2);
			}
		else
			{
			LEDOption(3);
			}
		}
}

//Change a value which is stored in the EEPROM at the given address
void SetupMinutes(unsigned char EEPROM_Addr, unsigned char minValue, unsigned char maxValue)
{
	unsigned char Value;
	Value = Read_EEPROM(EEPROM_Addr);
	LCD_MinuteOff(Value, maxValue);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&Value, maxValue, minValue))
			{
			LCD_MinuteOff(Value, maxValue);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEPROM_Addr, Value);
}

//change ad Brigthness which is stored at the given EEPROM address
void SetupBrightness(unsigned char EEPROM_Addr)
{
	signed char tempBrightness;
	tempBrightness=Brightness;		//store current brightness
	Brightness = Read_EEPROM(EEPROM_Addr);
	PWM_SetupNow( 0);			//setup brightness
	LCD_SendBrightness();
	while(CheckKeyPressed())
		{
		// A Rotation occured if EncoderSteps!= 0
		// EncoderSteps = 0 must be set by receiving program after decoding
		if (EncoderSteps)
			{
			PWM_SetupNow(EncoderSteps);
			EncoderSteps = 0;		//ack received steps
			LCD_SendBrightness();
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEPROM_Addr, Brightness);
	Brightness=tempBrightness;
	PWM_SetupNow(0);
}

void LCD_Contrast(unsigned char Contrast)
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Contrast %2d", Contrast);
	LEDOption(Contrast >> 2 & 0x03);
}

//change contrast setting of LCD and store in EEPROM
void SetupContrast()
{
	unsigned char Contrast;
	Contrast = Read_EEPROM(EEAddr_LCDContrast);
	LCD_Contrast(Contrast);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&Contrast, maxContrast, minContrast))
			{
			LCD_SetContrast(Contrast);
			LCD_Contrast(Contrast);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEAddr_LCDContrast, Contrast);
}

void LCD_SetupRCAddress(unsigned char Address)
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Addr %2d         ", Address);
	LEDOption(0);
}

//change RC5 address by receiving a now one and store it after a key being pressed in the EEPROM
void SetupRCAddress()
{
	unsigned char Address;
	Address = Read_EEPROM(EEAddr_RC5Addr);
	LCD_SetupRCAddress(Address);
	while(CheckKeyPressed())
		{
		if (12==rCounter)
			{
			if((RC5Addr_front!=rAddress) && (RC5Addr_back!=rAddress) && (RC5Addr_com!=rAddress))	//do not use inter lamp com channels
				{
				Address=rAddress;
				LCD_SendCmd(LCDSet2ndLine);
				printf_fast("Addr %2d Cmd %2d %1d", Address, rCommand, RTbit);
				LEDOption(1);
				rCounter=0;		//Nach Erkennung zurücksetzen
				}
			}
		else if (EncoderSetupValue(&Address, maxRC5Address,0))
			{
			LCD_SetupRCAddress(Address);
			}

		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEAddr_RC5Addr, Address);	//Update EEPROM
	RC5Addr=Address;				//Update RAM
	EncoderSteps = 0;				//reset steps
}

void LCD_InitEEPROMYN(unsigned char j)
{
	LCD_SendString2ndLine("Reset? ");
	LCD_SendString(&noyestext[j][0]);
	LEDOption(j);
}

//reset EEPROM to default values
void InitEEPROM()					//reset EEPROM to default
{
	unsigned char j = 0;			//set to "no" = false
	LCD_InitEEPROMYN(j);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&j, 1,0))
			{
			LCD_InitEEPROMYN(j);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	if (j)
		{
		for(j=0; j<sizeEEPROM; j++)
			{
			Write_EEPROM(j, initEEPROMdata[j]);
			}
		//update values
		LCD_SetContrast(Read_EEPROM(EEAddr_LCDContrast));
		PWM_SetupNow(0);
		}
}

void LCD_ComMode(unsigned char j)
{
	LCD_SendString2ndLine(&ComModetext[j][0]);
	LEDOption(j);
}

//Setup communication mode
void SetupComMode(unsigned char EEPROM_Address)
{
	unsigned char Mode;
	Mode = Read_EEPROM(EEPROM_Address);
	LCD_ComMode(Mode);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&Mode, maxComMode, 0))
			{
			LCD_ComMode(Mode);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEPROM_Address, Mode);
}

//Display the current Option
void LCD_CurrentOption(unsigned char Option)
{
	LCD_SendString2ndLine(&OptionNames[Option][0]);
	LEDOptionsFlash(Option);
}

void LCD_Option(unsigned char Option)
{
	LCD_ClearDisplay();
	LCD_SendString("Options");
	LCD_CurrentOption(Option);
}

/** options menu loop */
void Options()
{
	unsigned char stay = 1;
	unsigned char Option = 0;

	OldKeyState = 0;
	EncoderSteps = 0;				//reset steps

	LCD_Option(Option);

	while(stay)
		{
		// Select key is pressed, show preview of action
		if (TimerFlag)
			{
			LEDFlashing();
			if (KeySelect == KeyState)
				{
				if (KeyPressShort == KeyPressDuration)
					{
					LEDFlashReset();
					LCD_SendStringFill2ndLine("Exit Options");
					if (LightOn)
						{
						LEDOn();
						}
						else
						{
						LEDStandby();
						}
					}
				else if (KeyPressLong == KeyPressDuration)
					{
					LCD_SendStringFill2ndLine(&Canceltext[0]);
					LEDCancel();
					}
				}
			TimerFlag = 0;		//acknowledge key pressing
			}

		// A Key was pressed if OldKeyState != 0 and Keystate = 0
		// OldKeyState = 0 must be set by receiving program after decoding as a flag
		else if ((KeySelect == OldKeyState) && (0 == KeyState))
			{
			if (KeyPressShort > KeyPressDuration)
				{
				OldKeyState=0;				//acknowldge key pressing
				LCD_ClearDisplay();			//prepare display for submenu
				LCD_SendString(&OptionNames[Option][0]);	// display option name in first line
				LEDFlashReset();				//prepare LED
				switch (Option)
					{
					case 0:
						SetupBrightness(EEAddr_MinimumFrontBrightness);
						break;
					case 1:
						PWM_Offset=0;
						SetupBrightness(EEAddr_OffsetFrontBrightness);
						Update_PWM_Offset();
						break;
					case 2:
						SetupBrightness(EEAddr_AlarmFrontBrightness);
						break;
					case 3:
						SetupMinutes(EEAddr_LightFading, minLightFading, maxLightFading);
						break;
					case 4:
						SetupRCAddress();
						break;
					case 5:
						SetupComMode(EEAddr_ReceiverMode);
						ReceiverMode=Read_EEPROM(EEAddr_ReceiverMode);
						break;
					case 6:
						InitEEPROM();
						break;
					case 7:
						SetupContrast();
						break;
					}
				LCD_Option(Option);	//Refresh display after setup function
				}
			else if (KeyPressLong > KeyPressDuration)
				{
				stay=0;
				}
			else
				{
				LCD_CurrentOption(Option);	//Cancel key pressing, refresh display
				}
			OldKeyState=0;			//acknowldge key pressing
			EncoderSteps = 0;			//reset steps
			}
		if (EncoderSetupValue(&Option, maxOption, 0))
			{
			LCD_CurrentOption(Option);
			}
		PCON=MCUIdle;				//go idel, wake up by any int
		}
	LCD_ClearDisplay();
	LEDFlashReset();
}