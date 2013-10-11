/* Function to obtain brightness
 * requires two volatile variables to store gain setting and value for moving average
 * call frequently
 * divide result by number of data points of the moving average
 * maximum filter size = 2^32 / 2^8 / 1000 = 167777 datapoints
 * result is found in the unsigned long ExtBrightness
 * V0 2012-11-04
 */

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>

#define minphotoamp	15
#define maxphotoamp	200
#define minphotogain	0b00
#define maxphotogain	0b11

unsigned long ExtBrightness;		//will be filled up to 24 bit with 64 datapoint moving average

// define the bitfield
typedef union {
	struct {
		unsigned ALL:8;  
	};
	struct {
		unsigned LSB:1; 
		unsigned MSB:1;
		unsigned free:6;
	};
} PhotoGain_t;
 
// declare var
PhotoGain_t PhotoGain;

/**amplification factors of photoamp */
__code unsigned int photoampfactor[] = {1, 10, 100, 1000};

void MeasureExtBrightness()
{
	unsigned char ADC_Result;

	ADCON0 = 0b00000101; 				//Enable ADC&DAC 0 immediate start
	ExtBrightness -= (ExtBrightness >> 6);		//in the meantime: remove a 1/64 so we have a moving average over 64 datapoints
	while (0 == ADCON0 & ADCI0) {}			//wait for ADC
	ADC_Result=AD0DAT3;
	ExtBrightness += ADC_Result * photoampfactor[PhotoGain.ALL];
	if ((maxphotoamp < ADC_Result) && (maxphotogain > PhotoGain.ALL))
		{
		++PhotoGain.ALL;
		}
	else if ((minphotoamp > ADC_Result) && (minphotogain < PhotoGain.ALL))
		{
		--PhotoGain.ALL;
		}
	P0_6 = PhotoGain.LSB;		//set gain
	P0_7 = PhotoGain.MSB;
}