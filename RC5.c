/* Function to decode and code RC5 commands
   call decoder state machine at 4499 Hz for four times oversampling of the 889µs demodulated RC5 signal pulses
 */

#define IntT0reload 0x04			//T0 extension to clock RC5 receiver

#define _RC5inp		P1_5		//define RC5 input pin
#define _CS_Flash		P0_5		//define /CS for serial FLASh and /Shuntown for audio amp
#define RC5output		P2.1		//define RC5 output pin, assembler style required

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
	__data static unsigned char IntT0Count;		//Verlängerung von T0
	__data static unsigned char rc5state;
	__bit static Rbit;				//Dedektiertes Bit von RC5

	--IntT0Count;
	if (0==IntT0Count)
		{
		IntT0Count=IntT0reload;			//Reload counter
		rc5state=rc5state<<1;
		if (0==_RC5inp)				//insert inverted input
			{
			rc5state|=1;
			}
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
		}
}


// RC5 Sender


//send IR Pulse as defined for RC5 with 36KHz modulation
#define IRPulse \
	__asm \
   	setb RC5output \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
	nop \
   	nop \
	nop \
	clr RC5output \
	__endasm;

//send zero
void SendBit0()
{
	unsigned char i;
 	for(i=0; i<32; i++)
	  	{
		IRPulse
		Wait2us(7);
	  	}
	//890us Pause
  	Wait100us(8);
  	Wait2us(43);
}

//Send one
void SendBit1()
{
	unsigned char i;
	//890us Pause
	Wait100us(8);
  	Wait2us(43);

 	//890us Impuls mit 36kHz senden
 	for(i=0; i<32; i++)
  		{
		IRPulse
		Wait2us(7);
  		}

}

//Sends RC5 code, disables all irs to keep the timing 
//timing for 8051 two clock core at 6MHz
void SendCommand(unsigned char address, unsigned char code, unsigned char toggle)
{
	unsigned char mask;
	unsigned char i;

 	EA=0;		//all irq off
	SendBit1();	//1st Startbit=1
	SendBit1();	//2nd Startbit=1

	//Togglebit
	if(toggle==0)
		{
		SendBit0();
    		}
   	else
    		{
     		SendBit1();
    		}

	//5 Bit Address
   	mask=0x10;	//Begin with MSB
   	for(i=0; i<5; i++)
    		{
		if(address&mask)
      			{
       			SendBit1();
      			}
     		else
      			{
       			SendBit0();
      			}
		mask>>=1;	//Next bit
    		}

	//6 Bit Code
   	mask=0x20;
	for(i=0; i<6; i++)
		{
		if(code&mask)
			{
       			SendBit1();
      			}
     		else
     	 		{
       			SendBit0();
      			}
     		mask>>=1; 
		}
	//switch off IR-LED anyway
	__asm
   		clr RC5output
   	__endasm;
   	EA=1;		//all irq on again
}


void CommandPause()
{
	// use all interupt sources to create delay of 89ms
	unsigned int j;
	for(j=1641; j; j--)
		{
		PCON=0b00000001;				//go idel, wake up by any int
		}
}

//Sends RC5 code if required, including repeats
void SendRC5(unsigned char address, unsigned char code, unsigned char toggle, unsigned char requiredmode, unsigned repeats)
{
	unsigned char j;
	if (SenderMode>=requiredmode)
		{
		for(j=1; j<=repeats; j++)
			{
			SendCommand(address, code, toggle);
			if (j<repeats)			//skip last pause in sequence of repeated commands
				{
				CommandPause();		//wait 88.9ms
				}
		   	}
	   	}
}

//setup brightness values
void SetLightRemote(unsigned char i, signed char steps)
{
	PWM_SetupDim(i+1, Brightness_steps, steps);
	LCD_SendBrightness(i+1);
	FocusBacklight=i;
	LightOn=1;
}

void SetBrightnessRemote(unsigned char i)
{
	if (ReceiverMode>=ComModeConditional)
		{
		Brightness[i+1]=((rCommand<<1) & 0x7E) + RTbit;
		SetLightRemote(i,0);
		}
}

//check 12==rCounter before calling!!!
void DecodeRemote()
{
	__bit static RTbitold;				//Togglebit des letzten Befehls von RC5

	if (RC5Addr==rAddress)
		{
		if (RTbit ^ RTbitold)			//Neue Taste erkannt
			{
			switch (rCommand)
				{
				case 1:
				case 12:			//Standby
					SwAllLightOn();
					break;
				case 13:			//mute
					SwAllLightOff();
					break;
  				}
			}

  		switch (rCommand)			//new or same key pressed
  			{
  			case 16:				//incr vol
				SetLightRemote(0, RemoteSteps);
				break;
  			case 17:				//decr vol
				SetLightRemote(0, -RemoteSteps);
				break;
			case 32:				//incr channel
				SetLightRemote(1, RemoteSteps);
				break;
			case 33:				//decr channel
				SetLightRemote(1, -RemoteSteps);
				break;
  			}
  		RTbitold=RTbit;				//Togglebit speichern
  		}
  	else if (RC5Addr_front==rAddress)
  		{
  		SetBrightnessRemote(0);
  		}
  	else if (RC5Addr_back==rAddress)
  		{
  		SetBrightnessRemote(1);
  		}
  	else if (RC5Addr_com==rAddress)
  		{
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
  					AlarmEnd();
					LCD_SendBrightness(FocusBacklight+1);
  					}
  				break;
  			case RC5Cmd_Off:
				if (ComModeConditional<=ReceiverMode)
					{
					SwAllLightOff();
					}
  				break;
  			case RC5Cmd_On:
				if (ComModeConditional<=ReceiverMode)
					{
					SwAllLightOn();
					}
  				break;
  			}
  		}
	rCounter=0;					//Nach Erkennung zurücksetzen
}