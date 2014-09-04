/**
 * Options of LED Leuchte incl. EEPROM access and Alarm
 * @file Options.h
 */

#ifndef __Options_H_GUARD
#define __Options_H_GUARD 1

// define EEPROM mapping
#define EEAddr_RC5Addr 0			//RC5 Address
#define EEAddr_LCDContrast 1		//LCD contrast setting
#define EEAddr_DispBrightness 2		//Brightness of the display backlight
#define EEAddr_LightFading	3		//Minutes to fade light in
#define EEAddr_AlarmFrontBrightness 4	//Brightness of the frontlight during alarm
#define EEAddr_AlarmBackBrightness 5	//Brightness of the backlight during alarm, address must follow AlarmFrontBrightness!!!
#define EEAddr_AlarmTime2Signal 6		//Delay after alarm until noise is being generated
#define EEAddr_AlarmTimeSnooze 7		//Snooze Time
#define EEAddr_ReceiverMode 8
#define EEAddr_SenderMode 9
#define EEAddr_MinimumFrontBrightness 10	//Brightness of the frontlight during power on
#define EEAddr_MinimumBackBrightness 11	//Brightness of the backlight during power on, address must follow MinimumFrontBrightness!!!
#define EEAddr_OffsetFrontBrightness 12	//Brightness Offset of the frontlight
#define EEAddr_OffsetBackBrightness 13	//Brightness Offset of the backlight
#define EEAddr_BeepVolume 14		//Volume of the key beep
#define EEAddr_AlarmOffset 16		//Address offset of the alarms

#define maxOption 18
#define maxAlarm 6

#define daysinweek 7
#define alldays 8

#define minLightFading 01
#define maxLightFading 40			//avoid overflow of AlarmDim_Cnt_Reload
#define minTime2Signal 00
#define maxTime2Signal 99			//avoid overflow of encodersetup value

#define minContrast 0x07
#define maxContrast 0x0F

#define sizeAlarm 3
#define sizeEEPROM (EEAddr_AlarmOffset+sizeAlarm*(maxAlarm+1))

#define maxAlarmEndMode 4
#define maxComMode 3

#define maxBeepVolume 4		// three volume levels plus off, relized in software

#define menutimeout 184630		//timeout = 10s defined by RC5 irq frequency

				//the following three values have to be held consistend manually!
#define skipAlarmStepping 3	//2^skipAlarmStepping number of steps required to change skipAlarmCnt, should multiplied by maxAlarm not exceed unsigned char
#define skipAlarmhalfStep 4	// = (2^(skipAlarmStepping-1)   savety margin to alow for backlight, but avoid alarm change
#define skipAlarmMask 0xF8		// = (0x100-(2^skipAlarmStepping))

unsigned int AlarmDim_Cnt[2] = {0,0};
unsigned int AlarmDim_Cnt_Reload[2] = {0,0};

unsigned char skipAlarmCnt;		//number of alarms to be skipped = 2^skipAlarmStepping
unsigned char maxskipAlarmCnt;		//number of active alarms * 2^skipAlarmStepping + skipAlarmhalfStep, must be set in Find_NextAlarm()

unsigned char RC5Addr;
unsigned char ReceiverMode;
unsigned char SenderMode;

__code unsigned char initEEPROMdata[sizeEEPROM] = {0, 12, 0x40, 16, 0x20, 0x80, 11, 6, 2, 2, 7, 7, 1, 1, 2 , 0,
						6, 0, 1, 6, 0, 2, 6, 0, 3, 6, 0, 4, 6, 0, 5, 8, 0, 6, 8, 0, 7};

__code char OptionNames[maxOption+1][15] = {"Set Alarm     ",
				 	 "Set Clock     ",
				 	 "Frontlight    ",
				 	 "Backlight     ",
				 	 "Light Fading  ",
				 	 "Time to Signal",
				 	 "Snooze Time   ",
				 	 "Receiver Mode ",
				 	 "Sender Mode   ",
				 	 "Min. Front    ",
				 	 "Min. Back     ",
				 	 "Offset Front  ",
				 	 "Offset Back   ",
				 	 "LCD Backlight ",
				 	 "LCD Contrast  ",
				 	 "Set RC Address",
				 	 "Key Beep Vol  ",
				 	 "Reset settings",
				 	 "V4 2014-02-22 "};

__code char noyestext[2][4] = {" no", "yes"};

__code char SnoozeEndtext[maxAlarmEndMode+1][15] = {"Keep Snooze   ", "End Alarm     ", "End All Alarm ", "Go Standby    ", "All Go Standby"};

__code char ComModetext[maxComMode+1][10] = {"Off      ", "Alarm    ", "Condional", "All      "};

__code char Alarmtext[] = "Alarm!";

__code char Canceltext[] = "Cancel";

#define ComModeOff	0
#define ComModeAlarm	1
#define ComModeConditional	2
#define ComModeAll	3

__code unsigned char minAlarmvalue[sizeAlarm]  = {  0,  0,  0};
__code unsigned char maxAlarmvalue[sizeAlarm]  = { 23, 59,  8};	//weekday == 0 => off, weekday == 8 => any weekday
__code unsigned char Alarmcursorpos[sizeAlarm] = {  1,  4,  8};

unsigned char Read_EEPROM(unsigned char address);

void Write_EEPROM(unsigned char address, unsigned char data2write);

void ReadAlarmEEPROM(unsigned char AlarmNo, unsigned char *curAlarm);

void WriteAlarmEEPROM(unsigned char AlarmNo, unsigned char *curAlarm);

void LCD_AlarmSnoozeEnd(unsigned char j);

//stop count down to acustic alarm
void AlarmEnd();

//menu to stop count down to acustic alarm
void AlarmSnoozeEnd();

//display an alarm from EEPROM in 2nd line
void LCD_SelectAlarm(unsigned char AlarmNo);

//display an alarm from ram in 2nd line including cursor setting
void LCD_SetupAlarm(unsigned char j, unsigned char *curAlarm);

//wake-up light dimming
void Alarm_StepDim(unsigned char i);

//wake-up light active
void Alarm_StepDim_all();

//prepare wake-up light
void SetupAlarmDim(unsigned char i);

void LCD_SnoozeTime();

//execute an alarm
void Alarm();

//check if any alarm is set to be excuted NOW
void CheckAlarm();

//Checks for the next alarm, returns 0 if no alram was found
unsigned char Find_NextAlarm();

//Checks for the next alarm and show it on the display
void LCD_NextAlarm();

//edit an alarm
void SetupAlarm(unsigned char AlarmNo);

//select an alarm to be edited
void SelectAlarm();

//display current clock values including year and cursor positioning
void LCD_SetupClock(unsigned char j);

//setup the RTC values by user, funny order of values to be setup is given by the RTC design
void SetupClock();

void LCD_MinuteOff(unsigned char Value);

//Change a value which is stored in the EEPROM at the given address
void SetupMinutes(unsigned char EEPROM_Addr, unsigned char minValue, unsigned char maxValue);

//change ad Brigthness j which is stored at the given EEPROM address
void SetupBrightness(unsigned char EEPROM_Addr, unsigned char j);

void LCD_Contrast(unsigned char Contrast);

//change contrast setting of LCD and store in EEPROM
void SetupContrast();

//change RC5 address by receiving a now one and store it after a key being pressed in the EEPROM
void SetupRCAddress();

void LCD_InitEEPROMYN(unsigned char j);

//reset EEPROM to default values
void InitEEPROM();

void LCD_ComMode(unsigned char j);

//Setup communication mode
void SetupComMode(unsigned char EEPROM_Address);

//Display the current Option
void LCD_CurrentOption(unsigned char Option);

void LCD_Option(unsigned char Option);

/** options menu loop */
void Options();

#endif