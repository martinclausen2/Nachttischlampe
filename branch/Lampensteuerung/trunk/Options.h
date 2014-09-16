/**
 * Options of LampenSteuerung incl. passive Alarm
 * @file Options.h
 */

#ifndef __Options_H_GUARD
#define __Options_H_GUARD 1

// define EEPROM mapping
#define EEAddr_RC5Addr 0			//RC5 Address
#define EEAddr_LCDContrast 1		//LCD contrast setting
#define EEAddr_LightFading	2		//Minutes to fade light in
#define EEAddr_AlarmFrontBrightness 3	//Brightness during alarm
#define EEAddr_ReceiverMode 4
#define EEAddr_MinimumFrontBrightness 5	//Minium Brightness during power on
#define EEAddr_OffsetFrontBrightness 6	//Brightness Offset of the frontlight
#define EEAddr_DetectorTimeout 7		//Time the lights starts to fade out after the motion detector input was triggered
#define EEAddr_DetectorBrightness 8		//Time the lights starts to fade out after the motion detector input was triggered
#define EEAddr_ExtBrightness_last_MSB 9	//MSB of the measured external brightness belonging to the last stored brightness setting
#define EEAddr_ExtBrightness_last_LSB 10	//LSB of the measured external brightness
#define EEAddr_CurrentBrightness 11		//last stored brightness setting

#define sizeEEPROM (EEAddr_CurrentBrightness+1)

#ifndef LCD
#define maxOption 8
#else
#define maxOption 10
#endif

#define minLightFading 01
#define maxLightFading 30

#define minDetectorTimeout 00		//0 = kein Bewegungssensor
#define maxDetectorTimeout 15

#define minDetectorBrightness 0
#define maxDetectorBrightness 0xFF

#define minContrast 0x05
#define maxContrast 0x0F

#define maxAlarmEndMode 4
#define maxComMode 3

#define menutimeout 184630			//timeout = 10s defined by RC5 irq frequency

unsigned int AlarmDim_Cnt = 0;
unsigned int AlarmDim_Cnt_Reload = 0;

unsigned char skipAlarmCnt;		//number of alarms to be skipped = 2^skipAlarmStepping
unsigned char maxskipAlarmCnt;		//number of active alarms * 2^skipAlarmStepping + skipAlarmhalfStep, must be set in Find_NextAlarm()

unsigned char RC5Addr;
unsigned char ReceiverMode;
unsigned char SenderMode;

__code unsigned char initEEPROMdata[sizeEEPROM] = {0, 12, 16, 0x20, 2, 7, 1, 6, 0x1F, 0x00, 0xFF, 0x7F};

#ifdef LCD
__code char OptionNames[maxOption+1][17] = {
				 	 "Min. Light      ",
				 	 "Offset Light    ",
					 "Light Alarm     ",
				 	 "Light Fading    ",
				 	 "Detector Timeout",
				 	 "Det.Bright.Limit",
				 	 "Set RC Address  ",
				 	 "Receiver Mode   ",
				 	 "Reset settings  ",
				 	 "LCD Contrast    ",
				 	 "V1 2014-09-15   "};

__code char noyestext[2][4] = {" no", "yes"};

__code char ComModetext[maxComMode+1][10] = {"Off      ", "Alarm    ", "Condional", "All      "};

__code char Canceltext[] = "Cancel";
#endif

#define ComModeOff	0
#define ComModeAlarm	1
#define ComModeConditional	2
#define ComModeAll	3

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

#ifdef LCD
void LCD_Contrast(unsigned char Contrast);

//change contrast setting of LCD and store in EEPROM
void SetupContrast();
#endif

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