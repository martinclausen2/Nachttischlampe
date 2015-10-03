/* Function to decode and code RC5 commands for LampenSteuerung
   call decoder state machine at 4499 Hz for four times oversampling of the 889µs demodulated RC5 signal pulses
 */

#define _RC5inp		P1_4		//define RC5 input pin

#define EncoderReload	8		//divides 4,5KHz down to 500Hz

#define RC5Addr_front	27		//address for frontlight brightness
#define RC5Addr_back	28		//address for backligth brightness, MUST follow front address!
#define RC5Addr_com	29		//address for RC5 communication
#define maxRC5Address	31		//maximum possible RC5 address

#define RC5Cmd_AlarmStart	53		//code for start of alarm
#define RC5Cmd_AlarmEnd	54		//code for end of alarm
#define RC5Cmd_Off	13		//code for light off
#define RC5Cmd_On		12		//code for light on

#define RC5Cmd_Repeats	3		//number of repeats for on, off and AlarmStart commands
#define RC5Value_Repeats	1		//number of repeats for value transmission

#define RemoteSteps	2		//Step size for brightness control via remote control
#define Brightness_steps	20		//number of steps used to execute a brightness change

__data volatile unsigned char rCommand;   //Bitte erhalten! Wenn Befehl fertig: auswerten
__data volatile unsigned char rAddress;   //Bitte erhalten! Wenn Befehl fertig: auswerten
__data volatile unsigned char rCounter;   //Bitte erhalten!

__bit volatile RTbit;			//Togglebit von RC5

//State Machine zur Decodierung des RC5-Fernbedieungscodes
__code unsigned char tblRemote[] =
{1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0,
 8, 0, 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14,
 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0,
 20, 21, 0, 22, 0, 23, 26, 24, 26, 25, 26, 0,
 27, 0, 28, 0, 29, 31, 30, 31, 0, 31, 0, 32,
 0, 33, 35, 34, 35, 43, 36, 43, 37, 0, 38, 40,
 39, 40, 0, 40, 0, 41, 0, 42, 35, 34, 35, 44,
 0, 45, 48, 46, 48, 47, 48, 0, 49, 0, 50, 0, 34, 43};

void T0_isr(void) __interrupt(1)	__using(isrregisterbank)	//int from Timer 0 to read RC5 state
{
	__data static unsigned char rc5state;
	__bit static Rbit;				//Dedektiertes Bit von RC5
	__data static unsigned char EncoderCounter;

	rc5state=rc5state<<1;
	rc5state|=!_RC5inp;
	rc5state=tblRemote[rc5state];
	if (33==rc5state)				//Startsequenz erkannt?
		{
		rCounter=0;			//alles zurücksetzen
		rAddress=0;
		rCommand=0;
		}
	else if ((42==rc5state) || (50==rc5state))	 //Erkanntes Bit einorden
		{
		if (42==rc5state)			//Nutzbit 1 erkannt?
			{
			Rbit=1;
			}
		else if (50==rc5state)		//Nutzbit 0 erkannt?
			{
			Rbit=0;
			}
   			++rCounter;			//Da neues Bit ...
		if (1==rCounter)
 				{
 				RTbit=Rbit;
 				}
		else if (7>rCounter)		//Adressbit
			{
 				rAddress=rAddress<<1;
 				rAddress|=Rbit;
 				}
		else				//Commandbit
			{
 				rCommand=rCommand<<1;
 				rCommand|=Rbit;
 				}
		}
	if (EncoderCounter)
		{
		EncoderCounter--;
		}
	else
		{
		EncoderCounter=EncoderReload;
		Encoder();
		}
}

//setup brightness values
void SetLightRemote(signed char steps)
{
	PWM_SetupDim(Brightness_steps, steps, 1);
	LEDSetupLimit();
	SendBrightness();
	WriteTimer=WriteTime;
}

void SetBrightnessRemote()
{
	if (ReceiverMode>=ComModeConditional)
		{
		Brightness=((rCommand<<1) & 0x7E) + RTbit;
		SetLightRemote(0);
		}
}

void SetBrightnessLevelRemote()
{
	if (rCommand<=10)
		{
		Brightness=(((unsigned int)rCommand*(unsigned int)maxBrightness)/10) & maxBrightness;
		SetLightRemote(0);
		}
}

void DecodeRemote()
{
	__bit static RTbitold;				//Togglebit des letzten Befehls von RC5

	if (12==rCounter)
		{
		if (RC5Addr==rAddress)
			{
			MotionDetectorTimer=0;		//reset any Motion Detector activity
			if (RTbit ^ RTbitold)			//Neue Taste erkannt
				{
				switch (rCommand)
					{
					case 12:			//Standby
						SwLightOn();
						break;
					case 13:			//mute
						SwLightOff();
						break;
					default:
						SetBrightnessLevelRemote();
						break;
	  				}
				}

	  		switch (rCommand)			//new or same key pressed
	  			{
	  			case 16:				//incr vol
					SetLightRemote(RemoteSteps);
					break;
	  			case 17:				//decr vol
					SetLightRemote(-RemoteSteps);
					break;
				case 32:				//incr channel
					SwLightOnMax();
					WriteTimer=WriteTime;
					break;
				case 33:				//decr channel
					SwLightOnMin();
					WriteTimer=WriteTime;
					break;
	  			}
	  		RTbitold=RTbit;				//Togglebit speichern
	  		}
	  	else if (RC5Addr_front==rAddress)
	  		{
			MotionDetectorTimer=0;	//reset any Motion Detector activity
	  		SetBrightnessRemote();
	  		}
	  	else if (RC5Addr_com==rAddress)
	  		{
			MotionDetectorTimer=0;	//reset any Motion Detector activity
	  		switch (rCommand)
	  			{
	  			case RC5Cmd_AlarmStart:
	  				if (ComModeAlarm<=ReceiverMode)
	  					{
	  					Alarm();
	  					}
	  				break;
	  			case RC5Cmd_AlarmEnd:
	  				if (ComModeAlarm<=ReceiverMode)
	  					{
						LCD_SendBrightness();
	  					}
	  				break;
	  			case RC5Cmd_Off:
					if (ComModeConditional<=ReceiverMode)
						{
						SwLightOff();
						}
	  				break;
	  			case RC5Cmd_On:
					if (ComModeConditional<=ReceiverMode)
						{
						SwLightOn();
						}
	  				break;
	  			}
	  		}
		rCounter=0;					//Nach Erkennung zurücksetzen
		}
}