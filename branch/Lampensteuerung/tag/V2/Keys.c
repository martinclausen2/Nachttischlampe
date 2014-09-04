/** Handels keys reading
 *  Hardware CCU of P89LPC93X
 *  V1 2013-01-20
 */

// Encoder and Keys should not overlap in their bits
#define KeysPort		P0		//select port with keys
#define SelKeys		0b00001100	//select keys within key port
#define KeyEscape		0b00000100
#define KeySelect		0b00001000

volatile unsigned char KeyState;		//stores the keys state
volatile unsigned char OldKeyState;
volatile unsigned char KeyPressDuration;


void WDT_RTC_isr(void) __interrupt(10) __using(isrregisterbank)	//int from internal RTC to update PWM read keys
{
	Flag40ms=1;					//notify main program at 25Hz
	KeyState = ~KeysPort & SelKeys;			//Get KeysState and invert it
	//decode keystate and measure time, no release debouncing, so do not call to often
	if (KeyState)					//Key pressed?
		{
		if (0 == OldKeyState)			//Is this key new?
			{
			OldKeyState = KeyState;		// then store it
			KeyPressDuration = 0;		//tick zero
			}
		else if (OldKeyState == KeyState) 
			{				//same key is pressed
			if (0xFF != KeyPressDuration)	//count ticks while same key is being pressed
				{
				++KeyPressDuration;
				}
			}
		else
			{				//we have a new key without an release of the old one
			OldKeyState = KeyState;		// store new one
			KeyPressDuration=0;		// reset time
			}
		}
	RTCCON = RTCen;					//Reset int flag manually
}

//Returns if to stay in that menu build with a while loop
unsigned char CheckKeyPressed()
{
	unsigned char returnval=1;
	// A Key was pressed if OldKeyState != 0 and Keystate = 0
	// OldKeyState = 0 must be set by receiving program after decoding as a flag
	if ((KeySelect == OldKeyState) && (0 == KeyState))
		{
		OldKeyState=0;
		returnval=0;
		}
	return returnval;
}