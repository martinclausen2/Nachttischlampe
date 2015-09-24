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
				//130 counts = 70°C
#define TempMax		175	//Temperature switch off in ADC counts
				//175 counts = 90°C
				//must go below TempLimit before reenabled
#define TempDerating	5	//Kehrwert Differenznormierung in 1/256

#define maxDisplayTimer	0xFF

unsigned int Temperature;		//will be filled up to 16 bit with 64 datapoint moving average

__bit overTemp;

void MeasureTemperature();

unsigned char LimitOutput();

unsigned char GetTemperature();

#ifdef LCD
void LCD_Temperature();
#endif

#endif