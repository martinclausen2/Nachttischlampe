/** Int routine for rotational encoder
 *
 * The calling frequency should not be to low, because otherwise the
 * state machine will be the confused by contact bouncing
 * e.g. T1 as 8 bit counter with 1:32 prescaler form 366Hz at 6MHz clk
 * or daisy chained with software timer from RC5 decoder
 * adding RC-filter may help to keep the calling frequency down
 *
 * A Rotation occured if 0!=EncoderSteps
 * EncoderSteps = 0 must be set by receiving program after decoding
 * Example
// if (EncoderSteps)
// {
// 	LCD_ClearDisplay();
//	printf_tiny("Encoder %d",  EncoderSteps);
//	EncoderSteps=0;
// }
*/

// Encoder and Keys should not overlap in their bits
#define EncoderPort	P1		//select port with encoder
#define SelEncoder 	0b00000011	//select Encoder bits, must be two lower bits
#define SelEncoderState	0b00011100	//select Enocder state bits
#define KeyIncr		0b00000001
#define KeyDecr		0b00000010
#define ShiftEncoderAccel	0x01		//max 7, depends on caling frequency of encoder decode function and required acceleration
#define SelEncoderAccel	0x7F		//depends of ShiftEncoderAccel
#define maxEncoderSteps	100		//avoid overflow
#define minEncoderSteps	-100
#define EncoderAccelStep	0x03		//depends on caling frequency of encoder decode function and required acceleration
#define maxEncoderAccel    0xFF
#define startEncoderDecay	0xAF		//start value for decrementing accelleration, depends on calling frequency

/** State Machine table for rotational encoder
   Format: IN  ACC.0 inc
              ACC.1 dec
              ACC.2 oldstate
              ACC.3 oldstate
              ACC.4 oldstate
          OUT ACC.0 inc, low active!
              ACC.1 dec, low active!
              ACC.2 state
              ACC.3 state
              ACC.4 state
*/

__code unsigned char tblEncoder[] = {
	0b00000011, 0b00000111, 0b00010011, 0b00000011,
	0b00001011, 0b00000111, 0b00000011, 0b00000011,
	0b00001011, 0b00000111, 0b00001111, 0b00000011,
	0b00001011, 0b00000011, 0b00001111, 0b00000001,
	0b00010111, 0b00000011, 0b00010011, 0b00000011,
	0b00010111, 0b00011011, 0b00010011, 0b00000011,
	0b00010111, 0b00011011, 0b00000011, 0b00000010
	};

volatile signed char EncoderSteps;

void Encoder()
{
	static unsigned char EncoderState;	//stores the encoder state machine state
	static unsigned char OldEncoderState;
	static unsigned char EncoderAccel;
	static unsigned char EncoderDecay;

	//construct new state machine input from encoder state and old state
	EncoderState &= SelEncoderState;
	EncoderState |= (SelEncoder & EncoderPort);
	EncoderState = tblEncoder[EncoderState];

	//Measure the number of steps, introduce acceleration
	if (KeyIncr == EncoderState)
		{
		if ((EncoderState == OldEncoderState) && (maxEncoderSteps > EncoderSteps))
			{
			if (maxEncoderAccel > EncoderAccel)
				{
				EncoderAccel +=EncoderAccelStep;
				}
			++EncoderSteps;
			EncoderSteps += (EncoderAccel >> ShiftEncoderAccel) & SelEncoderAccel;
			}
		else
			{
			EncoderSteps = 1;			//change in direction
			OldEncoderState = EncoderState;
			EncoderAccel = 0;
			}
		}
	else if (KeyDecr == EncoderState)
		{
		if ((EncoderState == OldEncoderState) && (minEncoderSteps < EncoderSteps))
			{
			if (maxEncoderAccel > EncoderAccel)
				{
				EncoderAccel +=EncoderAccelStep;
				}
			--EncoderSteps;
			EncoderSteps -= (EncoderAccel >> ShiftEncoderAccel) & SelEncoderAccel;
			}
		else
			{
			EncoderSteps = -1;		//change in direction
			OldEncoderState = EncoderState;
			EncoderAccel = 0;
			}
		}
	else if (EncoderDecay)
		{
		--EncoderDecay;
		}
	else
		{
		EncoderDecay=startEncoderDecay;
		if (EncoderAccel)
			{
			--EncoderAccel;			//no new step, decrease acceleration
			}
		}
}

//check encoder and change value within limits, returns true if value was changed
unsigned char EncoderSetupValue(unsigned char *Value, unsigned char maxValue, unsigned char minValue)
{
	signed int temp;
	unsigned char returnval;
	// A Rotation occured if EncoderSteps!= 0
	// EncoderSteps = 0 must be set by receiving program after decoding
	if (EncoderSteps)
		{
		temp = EncoderSteps;
		EncoderSteps = 0;			//ack received steps
		temp += *Value;
		if (maxValue < temp)
			{
			*Value = maxValue;
			}
		else if (minValue > temp)
			{
			*Value = minValue;
			}
		else
			{
			*Value = temp & 0xFF;
			}
		returnval = 1;
		}
		else
		{
		returnval = 0;
		}
	return returnval;
}