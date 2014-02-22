/* Function to communicate with RTC via I2C
 * V2 2013-05-09
 */

// TODO: handel "exceptions" from I2C state machine and move to int based operation

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>

#define I2C_Con_MasterEn	0b01000100	//Enable I2C, use SCL generator, send ack
#define I2C_Con_Disable	0b00000000	//Disable I2C
#define I2C_Con_Start	0b01100100	//Enable I2C and set start condition, use SCL generator, send ack
#define I2C_Con_Stop	0b01010100	//Enable I2C and set stop condition, use SCL generator, send ack
#define I2C_Con_StopStart	0b01110100	//Enable I2C and set first stop then start condition, use SCL generator, send ack

#define I2C_RTCControlPage 0x00		//Setup RTC
#define I2C_RTCIntFPage	0x02		//Clear RTC int flag
#define I2C_RTCStatusPage	0x03		//RTC Status
#define I2C_RTCClkPage	0x08		//Starts with seconds!
#define I2C_RTCTimerPage	0x18		//Setup timer

#define I2C_RTCTimerLow	59		//Reload every n+1 cycles, here each minute
#define I2C_RTCTimerHigh   0

#define I2C_RTCSetupDef	0b01011000	//Default Setup RTC without timer,required to setup the timer reload value and time
#define I2C_RTCSetupwithT	0b01011111	//Setup RTC with timer at 1Hz input
#define I2C_RTCSetupInt	0b00000010	//Int only from countdown timer

#define I2C_RTCAddrR	0xAD		//Read from RTC address
#define I2C_RTCAddrW	0xAC		//Write to RTC address

#define I2C_Stat_Start	0x08		//Start send
#define I2C_Stat_StartRep	0x10		//Start repeat send, switch to receiving mode
#define I2C_Stat_SlAWAck	0x18		//Slave address send with Write, ack received
#define I2C_Stat_SlAWNAck	0x20		//Slave address send with write, but ack received
#define I2C_Stat_DatTAck	0x28		//Data transmitted, ack received
#define I2C_Stat_DatTNAck	0x30		//Data transmitted, but no ack received
#define I2C_Stat_ArbLost	0x38		//Lost bus to other master
#define I2C_Stat_SlARAck	0x40		//Slave address send with Read, ack received
#define I2C_Stat_SlARNAck	0x48		//Slave address send with Read, but ack received
#define I2C_Stat_DatRAck	0x50		//Data received, ack send
#define I2C_Stat_DatRNAck	0x58		//Data received, Nack send

#define sizetimedate	7

unsigned char timedate[sizetimedate];	//array containing current time and date
					// format: 0 seconds, 1 minutes, 2 hours, 3 days, 4 weekdays, 5 months, 6 years

__code unsigned char mintimedatevalue[sizetimedate] =  { 0,  0,  0,  1,  1,  1, 13};
__code unsigned char maxtimedatevalue[sizetimedate] =  {59, 59, 23, 31,  7, 12, 79};
__code unsigned char timedatecursorpos[sizetimedate] = { 0,  4,  1, 11,  8, 14,  0};

__code char WeekdayNames[9][6] = {"off", "Mon","Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "all"};

void I2C_RTC_isr(void) __interrupt(2)	//served with lower priority, do NOT use the register bank of the other isr
{
	RefreshTimeRTC = 1;
}

void I2C_RAddr(unsigned char ReadAddress)
{
	I2DAT = ReadAddress;
	I2CON = I2C_Con_MasterEn;
	while(I2STAT != I2C_Stat_SlARAck){}
}

void I2C_WAddr(unsigned char WriteAddress)
{
	I2DAT = WriteAddress;
	I2CON = I2C_Con_MasterEn;
	while(I2STAT != I2C_Stat_SlAWAck){}
}

void I2C_TData(unsigned char data2send)
{
	I2DAT = data2send;
	I2CON = I2C_Con_MasterEn;
	while(I2STAT != I2C_Stat_DatTAck){}
}

void I2C_RData()					//Data can be found in I2Dat
{
	I2CON = I2C_Con_MasterEn;
	while(I2STAT != I2C_Stat_DatRAck){}
}

void I2C_Start(unsigned char Address)
{
	I2CON = I2C_Con_MasterEn;
	I2CON = I2C_Con_Start;
	while(I2STAT != I2C_Stat_Start){}
	I2C_WAddr(Address);
}

void I2C_Stop()
{
	I2CON = I2C_Con_Stop;
	I2CON = I2C_Con_Disable;
}

void I2C_StopStart()
{
	I2CON = I2C_Con_StopStart;
	while(I2STAT != I2C_Stat_Start){}
}

void GetTimeDateRTC()
{
	unsigned char j;
	//Reset all interupt flags
	I2C_Start(I2C_RTCAddrW);
	I2C_TData(I2C_RTCIntFPage);
	I2C_TData(0);
	I2C_StopStart();

	//Read Clock via I2C
	I2C_WAddr(I2C_RTCAddrW);
	I2C_TData(I2C_RTCClkPage);
	I2C_StopStart();

	I2C_RAddr(I2C_RTCAddrR);
	for (j=0; j<sizetimedate; j++)
		{							//Read time and date
		I2C_RData();
		timedate[j] = (I2DAT & 0x0F) + (((I2DAT & 0xF0)>>4)*10); 	//convert from BCD
		}
	I2C_Stop();
}

void WriteTimeDateRTC()
{
	unsigned char j;

	//Write control values to RTC and disable timer with int to make setting of timer possible
	I2C_Start(I2C_RTCAddrW);
	I2C_TData(I2C_RTCControlPage);
	I2C_TData(I2C_RTCSetupDef);
	I2C_StopStart();

	//write clock values
	I2C_WAddr(I2C_RTCAddrW);
	I2C_TData(I2C_RTCClkPage);
	for (j=0; j<sizetimedate; j++)
		{
		I2C_TData(((timedate[j]%10)&0x0F) + (((timedate[j]/10) & 0x0F)<<4));		//convert to BCD
		}
	I2C_StopStart();

	//Write timer values to RTC
	I2C_WAddr(I2C_RTCAddrW);
	I2C_TData(I2C_RTCTimerPage);
	I2C_TData(I2C_RTCTimerLow);
	I2C_TData(I2C_RTCTimerHigh);
	I2C_StopStart();

	//Write control values to RTC and enable timer with int
	I2C_WAddr(I2C_RTCAddrW);
	I2C_TData(I2C_RTCControlPage);
	I2C_TData(I2C_RTCSetupwithT);
	I2C_TData(I2C_RTCSetupInt);	//Setup Int
	I2C_TData(0);			//Reset Int Flags
	I2C_TData(0);			//Reset Status Flags (PON!)
	I2C_Stop();

}

void SetupRTC()	//setup the RTC chip after battery change
{
	// define the bitfield
	typedef union {
		struct {
			unsigned ALL:8; 
		};
		struct {
			unsigned X0:1;   
			unsigned X1:1;
			unsigned V1F:1;
			unsigned V2F:1;
			unsigned SR:1;
			unsigned PON:1;
			unsigned X3:1;
			unsigned EEbusy:1;
		};
	} RTC_Status_t;
 
	// declare var
	RTC_Status_t RTC_Status;

	unsigned char j;

	//Read Pon Flag and reset clock if required
	I2C_Start(I2C_RTCAddrW);
	I2C_TData(I2C_RTCStatusPage);
	I2C_StopStart();
	I2C_RAddr(I2C_RTCAddrR);
	I2C_RData();
	RTC_Status.ALL = I2DAT;
	I2C_Stop();

	if (RTC_Status.PON)			//Select Power On Flag
	{
		for (j=0; j<sizetimedate; j++){	//Reset time and date
		timedate[j] = mintimedatevalue[j];
		}
		WriteTimeDateRTC();
	}
}

void LCD_SendTime()
{
	LCD_ReturnHome();
	printf_fast("%2d:%02d ",timedate[2],timedate[1]);
	LCD_SendString(&WeekdayNames[timedate[4]][0]);
	printf_fast(" %2d.%2d.    ",timedate[3],timedate[5]);
}