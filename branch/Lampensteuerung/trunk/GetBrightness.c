/* Function to obtain brightness in LampenSteuerung
 * requires two volatile variables to store gain setting and value for moving average
 * call frequently
 * divide result by number of data points of the moving average
 * maximum filter size = 2^32 / 2^8 / 1000 = 167777 datapoints
 * result is found in the unsigned long ExtBrightness
 * V0 2014-08-31
 */

#define BrightexpPointsToAvg	6	//points to average in 2^x format
#define BrightInitCycles		0xFF	//at least 2^x

#define minphotoamp	15
#define maxphotoamp	200
#define minphotogain	0b00
#define maxphotogain	0b10

unsigned long ExtBrightness;		//will be filled up to 24 bit with 64 datapoint moving average

/**amplification factors of photoamp */
__code unsigned char PhotoGainTable[3] = {0b01100000, 0b10100000, 0b11000000};	//low "active", use only 3 upper MSB
__code unsigned int photoampfactor[3] = {1, 33, 1000};

void MeasureExtBrightness()
{
	unsigned char static PhotoGain;
	unsigned char ADC_Result;
	ADCON1 = 0b00000101; 			//Enable ADC&DAC 1 immediate start
						//in the meantime: remove a 1/64 so we have a moving average over 64 datapoints
	ExtBrightness -= (ExtBrightness >> BrightexpPointsToAvg) & 0x03FFFF;
	while (0 == ADCON1 & ADCI1) {}		//wait for ADC
	ADC_Result=0xFF-AD1DAT0;			//fetch & reverse
	ExtBrightness += ADC_Result * photoampfactor[PhotoGain];
	if ((maxphotoamp < ADC_Result) && (maxphotogain > PhotoGain))
		{
		++PhotoGain;
		}
	else if ((minphotoamp > ADC_Result) && (minphotogain < PhotoGain))
		{
		--PhotoGain;
		}

	//switch current sources on and off by toggeling Port between Push/Pull and Open Drain
	P0M1 = PhotoGainTable[PhotoGain] | P0M1def;
}

void InitMeasureExtBrightness()
{
	unsigned char i;

	for(i = BrightInitCycles; i; i--)
		{
		MeasureExtBrightness();
		PCON=MCUIdle;		//some delay, some interupts should be active
		}
}