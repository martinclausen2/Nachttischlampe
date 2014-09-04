/* Function to obtain temperature
 * requires one volatile variable to store value for moving average
 * call frequently
 * divide result by number of data points of the moving average
 * maximum filter size = 2^32 / 2^8 / 1000 = 167777 datapoints
 * result is found in the unsigned long Temperature
 * V0 2014-08-30
 */

// This file defines registers available in P89LPC93X
#ifndef __GetTemperature_H_GUARD
#define __GetTemperature_H_GUARD 1

#include <p89lpc935_6.h>

#define TempexpPointsToAvg	6	//points to average in 2^x format
#define TempScaling	225	// in 1/512 °C/ADC count
#define TempOffset	13	// in °C

#define TempLimit		130	//onset of output limiting in ADC counts
#define TempMax		175	//Temperature switch off in ADC counts
				//must go below TempLimit before reenabled
#define TempDerating	5	//kehrwert Differenznormierung in 1/256

unsigned int Temperature;		//will be filled up to 16 bit with 64 datapoint moving average

__bit overTemp;

void MeasureTemperature();

unsigned char LimitOutput();

unsigned char GetTemperature();

#endif