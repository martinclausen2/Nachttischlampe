/**
 * Options of LED Leuchte incl. EEPROM access and Alarm
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

void ReadAlarmEEPROM(unsigned char AlarmNo, unsigned char *curAlarm)
{
	unsigned char j;
	for(j=0; j<sizeAlarm; j++)
		{
		*(curAlarm+j)=Read_EEPROM(EEAddr_AlarmOffset+AlarmNo*sizeAlarm+j);
		}
}

void WriteAlarmEEPROM(unsigned char AlarmNo, unsigned char *curAlarm)
{
	unsigned char j;
	for(j=0; j<sizeAlarm; j++)
		{
		Write_EEPROM(EEAddr_AlarmOffset+AlarmNo*sizeAlarm+j,*(curAlarm+j));
		}
}

void LCD_AlarmSnoozeEnd(unsigned char j)
{
	RC5Addr=Read_EEPROM(EEAddr_RC5Addr);
	LCD_SendString2ndLine(&SnoozeEndtext[j][0]);
}

//stop count down to acustic alarm
void AlarmEnd()
{
	Minutes2Signal=0;
	RefreshTime=1;
}

//menu to stop count down to acustic alarm
void AlarmSnoozeEnd()
{
	unsigned long i;
	unsigned char j = 0;			//set to "snooze" = 0
	LCD_AlarmSnoozeEnd(j);

	for(i=menutimeout; i; i--)
		{
		if (TimerFlag)
			{
			PWM_StepDim();		// do next dimming step to permit LCD backlight fadein to finish
			TimerFlag=0;
			}

		if((!CheckKeyPressed()) || ((12==rCounter) && (RC5Addr==rAddress || RC5Addr_front==rAddress || RC5Addr_back==rAddress || RC5Addr_com==rAddress)))
			{
			break;			//exit on key pressed or RC5 command received
			}
		else if (EncoderSetupValue(&j, maxAlarmEndMode,0))
			{
			LCD_AlarmSnoozeEnd(j);
			i=menutimeout;		//reset timeout due to user action
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	// 0==j, nothing to do: timeout, RC5 command received or user keeps snooze
	// 0==i, timeout
	if (0!=j && 0!=i)			//end alarm
		{
		AlarmEnd();
		if (2==j || 4==j)
			{			//send end alarm via RC5, send anyway since use selected to do so
			SendRC5(RC5Addr_com, RC5Cmd_AlarmEnd, 1, ComModeOff, RC5Cmd_Repeats);
			}
		if (4==j)
			{			//send standby via RC5, send anyway since use selected to do so
			CommandPause();		//wait after sending AlarmEnd required
			SendRC5(RC5Addr_com, RC5Cmd_Off, 1, ComModeOff, RC5Cmd_Repeats);
			}
		if (3<=j)
			{			//goto standby
			SwAllLightOff();
			}
		if (3>j)
			{
			LCD_SendBrightness(FocusBacklight+1);
			}
		}
	RefreshTime=1;
}

//display an alarm from EEPROM in 2nd line
void LCD_SelectAlarm(unsigned char AlarmNo)
{
	unsigned char curAlarm[3];
	ReadAlarmEEPROM(AlarmNo, &curAlarm[0]);
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("#%d %2d:%02d ", AlarmNo, *curAlarm,*(curAlarm+1));
	LCD_SendString(&WeekdayNames[*(curAlarm+2)][0]);
	if(skipAlarmCnt&skipAlarmMask)		//show skipped alarms, if present
		{
		printf_fast(" S#%d ", (skipAlarmCnt>>skipAlarmStepping));
		}
	else
		{
		printf_fast("      ");
		}
}

//display an alarm from ram in 2nd line including cursor setting
void LCD_SetupAlarm(unsigned char j, unsigned char *curAlarm)
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("%2d:%02d  ", *curAlarm,*(curAlarm+1));
	LCD_SendString(&WeekdayNames[*(curAlarm+2)][0]);
	LCD_SendCmd(LCDSet2ndLine+Alarmcursorpos[j]);
}

//wake-up light dimming
void Alarm_StepDim(unsigned char i)
{
	if (AlarmDim_Cnt[i])
		{
		--AlarmDim_Cnt[i];				//count down step
		}
	else							//dimming step
		{
		if (Brightness[i+1] < Read_EEPROM(EEAddr_AlarmFrontBrightness+i))
			{
			AlarmDim_Cnt[i]=AlarmDim_Cnt_Reload[i];	//reload countdown
			PWM_SetupDim(i+1, Brightness_steps, 1);	//setup brightness
			}
		else
			{
			Alarmflag=0;				//we reached targetbrightness!
			}
		}
}

//wake-up light active
void Alarm_StepDim_all()
{
	if (Alarmflag)
		{
		Alarm_StepDim(0);
		Alarm_StepDim(1);
		}
}

//prepare wake-up light
void SetupAlarmDim(unsigned char i)
{
	AlarmDim_Cnt_Reload[i]=(Read_EEPROM(EEAddr_LightFading)*25*60)/Read_EEPROM(EEAddr_AlarmFrontBrightness+i);
	AlarmDim_Cnt[i]=AlarmDim_Cnt_Reload[i];
}

void LCD_SnoozeTime()
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Snooze %d Min   ", Minutes2Signal-1);
}

//execute an alarm
void Alarm()
{
	if (0==Alarmflag)
		{
		SetupAlarmDim(0);		// fade in to required brightness in on or off,
					// no fade in if already brighter, Alarm_StepDim() takes care of this behavior
		SetupAlarmDim(1);
		LightOn=1;
		Alarmflag=1;
		Minutes2Signal=Read_EEPROM(EEAddr_AlarmTime2Signal);
		if (Minutes2Signal)
			{
			LCD_SnoozeTime();
			}
		else
			{
			LCD_SendString2ndLine(&Alarmtext[0]);
			}
		}
}

//check if any alarm is set to be excuted NOW, update number of skipped alarms as required
void CheckAlarm()
{
	unsigned char curAlarm[3];
	unsigned char j;
	for(j=0; j<maxAlarm+1; j++)
		{
		ReadAlarmEEPROM(j, &curAlarm[0]);
		if(curAlarm[0]==timedate[2] && curAlarm[1]==timedate[1] && (curAlarm[2]==timedate[4] || alldays==curAlarm[2]))
			{
			if(skipAlarmCnt & skipAlarmMask)
				{
				skipAlarmCnt=(skipAlarmCnt & skipAlarmMask) - skipAlarmhalfStep;
				}
			else
				{
				SendRC5(RC5Addr_com, RC5Cmd_AlarmStart, 1, ComModeAlarm, RC5Cmd_Repeats);
				Alarm();
				}
			break;
			}
		}
}


unsigned char Find_NextAlarm()		//Checks for the next alarm, returns 0 if no alarm was found
					//considders skipAlarmCnt, not fast, but memory effective
{
	unsigned char curAlarm[3];
	unsigned char j;
	unsigned char i;
	signed int curdifference;		//time between alarm and curent time
	signed int mindifference=0;	//maximum difference is 7*24*60 Minutes = 10080 Minutes
					//0 is no alarm on yet found
	signed int curmindifference=0;	//minimum for this skipped alarm iteration, goes up with every skip iteration

	unsigned char minAlarm=0;		//number of the alarm with the smallest difference

	for(i=0; i<=(skipAlarmCnt>>skipAlarmStepping); i++)  //ignore compiler warning here
		{
		maxskipAlarmCnt=0;
		for(j=0; j<maxAlarm+1; j++)		//go through all alarms
			{
			ReadAlarmEEPROM(j, &curAlarm[0]);
			if (curAlarm[2])			//is this alarm on at all?
				{
				//calculate time difference between now and alarm
				if (alldays==curAlarm[2])
					{
					curdifference = 0;
					}
				else
					{
					curdifference = (curAlarm[2]-timedate[4])*24*60;
					}
				curdifference += (curAlarm[0]-timedate[2])*60;
				curdifference += curAlarm[1]-timedate[1];
				if (0>curdifference)		//warp into next week
					{
					curdifference+=daysinweek*24*60;
					}
				maxskipAlarmCnt++;

				// set alarm no if alarm is closer or no alarm is set yet
				if (((mindifference>curdifference) || 0==mindifference) && curmindifference<curdifference)
					{
					mindifference=curdifference;
					minAlarm=j+1;		//shift result by one => no alarm = 0
					}
				}
			}
		curmindifference=mindifference;			//save this iterations smallest time difference
		mindifference=0;					//reset search
		}
	maxskipAlarmCnt=(maxskipAlarmCnt<<skipAlarmStepping)+skipAlarmhalfStep;		//do not allow to skip more alarms than active with in one week
	return minAlarm;
}

void LCD_NextAlarm()			//Checks for the next alarm and show it on the display
{
	unsigned char AlarmNo;		//number of the next alarm
	AlarmNo=Find_NextAlarm();
	if (AlarmNo)			//do we have any alarm on at all?
		{
		LCD_SelectAlarm(AlarmNo-1);
		}
	else
		{
		LCD_SendStringFill2ndLine("Standby");
		}
}

//edit an alarm
void SetupAlarm(unsigned char AlarmNo)
{
	unsigned char curAlarm[3];
	unsigned char j=0;

	ReadAlarmEEPROM(AlarmNo, &curAlarm[0]);
	LCD_ClearDisplay();
	printf_fast("Alarm %d ", AlarmNo);
	LCD_SetupAlarm(j, &curAlarm[0]);
	LCD_SendCmd(LCDCursorOn);
	for(j=0; j<sizeAlarm; j++)
		{
		LCD_SetupAlarm(j, &curAlarm[0]);
		while(CheckKeyPressed())
			{
			if (EncoderSetupValue(&curAlarm[j], maxAlarmvalue[j], minAlarmvalue[j]))
				{
				LCD_SetupAlarm(j, &curAlarm[0]);
				}
			PCON=MCUIdle;		//go idel, wake up by any int
			}
		}
	WriteAlarmEEPROM(AlarmNo, &curAlarm[0]);
	LCD_SendCmd(LCDDisplayOn);			//Cursor off
}

//select an alarm to be edited
void SelectAlarm()
{
	unsigned char stay = 1;
	unsigned char AlarmNo;
	skipAlarmCnt=0;				//clear skipped alarms
	AlarmNo = Find_NextAlarm();		//this selects the alarm on the next day!
	if (AlarmNo)				//shift back, not found = 0: use first alarm
		{
		--AlarmNo;
		}

	LCD_SelectAlarm(AlarmNo);
	while(stay)
		{
		// Select key is pressed, show preview of action
		if (TimerFlag && KeySelect == KeyState)
			{
			if (KeyPressShort == KeyPressDuration)
				{
				LCD_SendStringFill2ndLine("Exit Alarms");
				}
			else if (KeyPressLong == KeyPressDuration)
				{
				LCD_SendStringFill2ndLine(&Canceltext[0]);
				}
			TimerFlag=0;
			}

		// A Key was pressed if OldKeyState != 0 and Keystate = 0
		// OldKeyState = 0 must be set by receiving program after decoding as a flag
		else if ((KeySelect == OldKeyState) && (0 == KeyState))
			{
			if (KeyPressShort > KeyPressDuration)
				{
				OldKeyState=0;		//acknowldge key pressing
				SetupAlarm(AlarmNo);
				LCD_ClearDisplay();
				LCD_SendString(&OptionNames[0][0]);
				LCD_SelectAlarm(AlarmNo);
				}
			else if (KeyPressLong > KeyPressDuration)
				{
				stay=0;
				}
			else
				{
				LCD_SelectAlarm(AlarmNo);
				}
			OldKeyState=0;
			EncoderSteps=0;
			}
		if (EncoderSetupValue(&AlarmNo, maxAlarm, 0))
			{
			LCD_SelectAlarm(AlarmNo);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
}

//display current clock values including year and cursor positioning
void LCD_SetupClock(unsigned char j)
{
	LCD_SendTime();
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Year 20%02d",timedate[6]);
	if (6!=j)
		{
		LCD_SendCmd(LCDSetAddress+timedatecursorpos[j]);
		}
	else
		{
		LCD_SendCmd(LCDSet2ndLine+8);
		}
}

//setup the RTC values by user, funny order of values to be setup is given by the RTC design
void SetupClock()
{
	unsigned char j;
	LCD_SendCmd(LCDCursorOn);
	for(j=1; j<7; j++)
		{
		LCD_SetupClock(j);
		while(CheckKeyPressed())
			{
			if (EncoderSetupValue(&timedate[j], maxtimedatevalue[j], mintimedatevalue[j]))
				{
				LCD_SetupClock(j);
				}
			PCON=MCUIdle;		//go idel, wake up by any int
			}
		}
	WriteTimeDateRTC();
	LCD_SendCmd(LCDDisplayOn);
}

void LCD_MinuteOff(unsigned char Value)
{
	LCD_SendCmd(LCDSet2ndLine);
	if (0==Value)
		{
		printf_fast("off       ");
		}
	else if (1==Value)
		{
		printf_fast("instantly ");
		}
	else if (2==Value)
		{
		printf_fast(" 1 Minute ");
		}
	else 
		{
		printf_fast("%2d Minutes", Value-1);
		}
}

//Change a value which is stored in the EEPROM at the given address
void SetupMinutes(unsigned char EEPROM_Addr, unsigned char minValue, unsigned char maxValue)
{
	unsigned char Value;
	Value = Read_EEPROM(EEPROM_Addr);
	LCD_MinuteOff(Value);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&Value, maxValue, minValue))
			{
			LCD_MinuteOff(Value);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEPROM_Addr, Value);
}

//change ad Brigthness j which is stored at the given EEPROM address
void SetupBrightness(unsigned char EEPROM_Addr, unsigned char j)
{
	signed char tempBrightness;
	tempBrightness=Brightness[j];		//store current brightness
	Brightness[j] = Read_EEPROM(EEPROM_Addr);
	PWM_SetupNow(j, 0);			//setup brightness
	LCD_SendBrightness(j);
	while(CheckKeyPressed())
		{
		// A Rotation occured if EncoderSteps!= 0
		// EncoderSteps = 0 must be set by receiving program after decoding
		if (EncoderSteps)
			{
			PWM_SetupNow(j, EncoderSteps);
			EncoderSteps = 0;		//ack received steps
			LCD_SendBrightness(j);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEPROM_Addr, Brightness[j]);
	if (0<j)					//fetch current brightness, except for backlight
		{
		Brightness[j]=tempBrightness;
		PWM_SetupNow(j, 0);
		}
}

void LCD_Contrast(unsigned char Contrast)
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Contrast %2d", Contrast);
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

void LCD_SetupBeepVolume(unsigned char Volume)
{
	LCD_SendCmd(LCDSet2ndLine);
	printf_fast("Volume %2d  ", Volume);
}

//change RC5 address by receiving a now one and store it after a key being pressed in the EEPROM
void SetupBeepVolume()
{
	unsigned char Volume;
	Volume = Read_EEPROM(EEAddr_BeepVolume);
	LCD_SetupBeepVolume(Volume);
	while(CheckKeyPressed())
		{
		if (EncoderSetupValue(&Volume, maxBeepVolume,0))
			{
			LCD_SetupBeepVolume(Volume);
			BeepVol(Volume);
			}
		PCON=MCUIdle;			//go idel, wake up by any int
		}
	Write_EEPROM(EEAddr_BeepVolume, Volume);	//Update EEPROM
	EncoderSteps = 0;				//reset steps
}

void LCD_InitEEPROMYN(unsigned char j)
{
	LCD_SendString2ndLine("Reset? ");
	LCD_SendString(&noyestext[j][0]);
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
		Brightness[0]=Read_EEPROM(EEAddr_DispBrightness);
		PWM_SetupNow(0, 0);
		}
}

void LCD_ComMode(unsigned char j)
{
	LCD_SendString2ndLine(&ComModetext[j][0]);
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
		if (TimerFlag && KeySelect == KeyState)
			{
			if (KeyPressShort == KeyPressDuration)
				{
				LCD_SendStringFill2ndLine("Exit Options");
				}
			else if (KeyPressLong == KeyPressDuration)
				{
				LCD_SendStringFill2ndLine(&Canceltext[0]);
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
				switch (Option)
					{
					case 0:
						SelectAlarm();
						break;
					case 1:
						SetupClock();
						break;
					case 2:
						SetupBrightness(EEAddr_AlarmFrontBrightness, 1);
						break;
					case 3:
						SetupBrightness(EEAddr_AlarmBackBrightness, 2);
						break;
					case 4:
						SetupMinutes(EEAddr_LightFading, minLightFading, maxLightFading);
						break;
					case 5:
						SetupMinutes(EEAddr_AlarmTime2Signal, minTime2Signal, maxTime2Signal);
						break;
					case 6:
						SetupMinutes(EEAddr_AlarmTimeSnooze, minTime2Signal, maxTime2Signal);
						break;
					case 7:
						SetupComMode(EEAddr_ReceiverMode);
						ReceiverMode=Read_EEPROM(EEAddr_ReceiverMode);
						break;
					case 8:
						SetupComMode(EEAddr_SenderMode);
						SenderMode=Read_EEPROM(EEAddr_SenderMode);
						break;
					case 9:
						SetupBrightness(EEAddr_MinimumFrontBrightness, 1);
						break;
					case 10:
						SetupBrightness(EEAddr_MinimumBackBrightness, 2);
						break;
					case 11:
						PWM_Offset[1]=0;
						SetupBrightness(EEAddr_OffsetFrontBrightness, 1);
						Update_PWM_Offset(1);
						break;
					case 12:
						PWM_Offset[2]=0;
						SetupBrightness(EEAddr_OffsetBackBrightness, 2);
						Update_PWM_Offset(2);
						break;
					case 13:
						SetupBrightness(EEAddr_DispBrightness, 0);
						break;
					case 14:
						SetupContrast();
						break;
					case 15:
						SetupRCAddress();
						break;
					case 16:
						SetupBeepVolume();
						break;
					case 17:
						InitEEPROM();
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
	RefreshTime=1;
	if (LightOn)
		{
		LCD_SendBrightness(FocusBacklight+1);
		}
}