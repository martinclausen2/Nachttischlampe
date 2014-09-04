/* Function to obtain temperature
 * requires one volatile variable to store value for moving average
 * call frequently
 * divide result by number of data points of the moving average
 * maximum filter size = 2^32 / 2^8 / 1000 = 167777 datapoints
 * result is found in the unsigned long Temperature
 * V0 2014-08-30
 */

// This file defines registers available in P89LPC93X
#include <p89lpc935_6.h>
#include "GetTemperature.h"

void MeasureTemperature()
{
	ADCON0 = 0b00000101; 			//Enable ADC&DAC 0 immediate start
						//in the meantime: remove a 1/2^x so we have a moving average over 2^x datapoints
	Temperature -= (Temperature >> TempexpPointsToAvg) & 0xFF;
	while (0 == ADCON0 & ADCI0) {}		//wait for ADC
	Temperature += AD0DAT1;
}

unsigned char LimitOutput()
{
	unsigned char value;
	value = (Temperature >> TempexpPointsToAvg) & 0xFF;

	if (value<TempLimit)
		{
		PWM_setlimited = PWM_set;
		overTemp = 0;
		return 0;
		}
	else if (value<TempMax)
		{
		if(overTemp)
			{
			return 2;
			}
		else
			{
			value = 0xFF - (value - TempLimit)* TempDerating;	//overflow must be avoided by correct settings
			PWM_setlimited = (((unsigned long)PWM_set*(unsigned long)value) >> 8) & 0x3FFF;
			return 1;
			}
		}
	else
		{
		PWM_setlimited = 0;
		overTemp = 1;
		return 2;
		}
}

unsigned char GetTemperature()
{
	unsigned int value;
	value = (Temperature >> TempexpPointsToAvg) & 0xFF;	//adjust accoring to points to average
	value = (value*TempScaling) >> 9 & 0x7F;		//scaling in 1/512, result can not be larger than 127°C
	value += TempOffset;				//offset in °C
	return (value & 0xFF);
}