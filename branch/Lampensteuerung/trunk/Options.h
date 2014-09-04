/**
 * Options of LampenSteuerung incl. EEPROM access and passive Alarm
 * @file Options.h
 */

#ifndef __Options_H_GUARD
#define __Options_H_GUARD 1

// define EEPROM mapping
#define EEAddr_RC5Addr 0			//RC5 Address
#define EEAddr_LCDContrast 1		//LCD contrast setting
#define EEAddr_LightFading	2		//Minutes to fade light in
#define EEAddr_AlarmFrontBrightness 3	//Brightness of the frontlight during alarm
#define EEAddr_ReceiverMode 4
#define EEAddr_MinimumFrontBrightness 5	//Brightness of the frontlight during power on
#define EEAddr_OffsetFrontBrightness 6	//Brightness Offset of the frontlight
#define EEAddr_ExtBrightness_last_MSB 7
#define EEAddr_ExtBrightness_last_LSB 8
#define EEAddr_CurrentBrightness 9

#define sizeEEPROM (EEAddr_CurrentBrightness+1)

#define maxOption 9

#define minLightFading 01
#define maxLightFading 40			//avoid overflow of AlarmDim_Cnt_Reload

#define minContrast 0x07
#define maxContrast 0x0F

#define maxAlarmEndMode 4
#define maxComMode 3

#define menutimeout 184630		//timeout = 10s defined by RC5 irq frequency

unsigned int AlarmDim_Cnt = 0;
unsigned int AlarmDim_Cnt_Reload = 0;

unsigned char skipAlarmCnt;		//number of alarms to be skipped = 2^skipAlarmStepping
unsigned char maxskipAlarmCnt;		//number of active alarms * 2^skipAlarmStepping + skipAlarmhalfStep, must be set in Find_NextAlarm()

unsigned char RC5Addr;
unsigned char ReceiverMode;
unsigned char SenderMode;

__code unsigned char initEEPROMdata[sizeEEPROM] = {0, 12, 16, 0x20, 2, 7, 1, 0x00, 0xFF, 0x7F};

__code char OptionNames[maxOption+1][15] = {
				 	 "Min. Light    ",
				 	 "Offset Light  ",
					 "Light Alarm   ",
				 	 "Light Fading  ",
				 	 "Set RC Address",
				 	 "Receiver Mode ",
				 	 "Reset settings",
				 	 "LCD Contrast  ",
				 	 "V0 2014-08-30 "};

__code char noyestext[2][4] = {" no", "yes"};

__code char ComModetext[maxComMode+1][10] = {"Off      ", "Alarm    ", "Condional", "All      "};

__code char Canceltext[] = "Cancel";

#define ComModeOff	0
#define ComModeAlarm	1
#define ComModeConditional	2
#define ComModeAll	3

unsigned char Read_EEPROM(unsigned char address);

void Write_EEPROM(unsigned char address, unsigned char data2write);

//wake-up light dimming
void Alarm_StepDim();

//wake-up light active
void Alarm_StepDim_all();

//prepare wake-up light
void SetupAlarmDim();

//execute an alarm
void Alarm();

void AlarmEnd();

void LCD_MinuteOff(unsigned char Value, unsigned char maxValue);

//Change a value which is stored in the EEPROM at the given address
void SetupMinutes(unsigned char EEPROM_Addr, unsigned char minValue, unsigned char maxValue);

//change a Brigthness which is stored at the given EEPROM address
void SetupBrightness(unsigned char EEPROM_Addr);

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