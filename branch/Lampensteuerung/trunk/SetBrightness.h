/** Sets out PWM value for LampenSteuerung
 *  Hardware CCU of P89LPC93X
 */

#ifndef __SetBrightness_H_GUARD
#define __SetBrightness_H_GUARD 1

#define maxBrightness	0x7F	//avoid overflow with signed numbers, should be filled with 1 from MSB to LSB
#define maxRawPWM		0x3FFF	// =  maxBrigntess^2
#define fadetime		75

#define WriteTime		0xFF	//time until new brightness value is saved to the eeprom
#define maxLimit		2	//upper brightness limit
#define minLimit		1	//lower brightness limit
				// no limit is 0

unsigned char WriteTimer;		//time until Brightness is saved in calls to StoreBrightness()
unsigned char limit;		//was brightness limit reached during last calculation?

unsigned char Brightness;		//current value
unsigned char Brightness_start;	//value before lights off
unsigned int PWM_Offset;		//PWM value, where the driver effectivly starts to generate an output
unsigned char Brightness_Offset;	//Brightness value, where the driver effectivly starts to generate an output
unsigned int ExtBrightness_last;	//external brigthness during lights off divided by 256

signed int PWM_set;		//current pwm value
signed int PWM_setlimited;	//current pwm value after limiter
signed int PWM_incr;		//pwm dimming step size
unsigned int PWM_incr_cnt;	//no of steps required to reach targed PWM value

void PWM_Set();

void LCD_SendBrightness();

void SendBrightness();

void Update_PWM_Offset();

void InitBrightness();

void SetExtBrightness_last();

void StoreBrightness();

void PWM_StepDim();			// perform next dimming step, must frquently called for dimming action

void PWM_SetupDim(signed int PWM_dimsteps, signed char Steps, signed int minBrightness);

void PWM_SetupNow(signed char Steps);

unsigned int sqrt32(unsigned long a);

void SwLightOn();

void SwLightOff();

void SwLightOnMax();

void SwLightOnMin();

#endif