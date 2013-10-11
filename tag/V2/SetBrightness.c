/** Sets all PWM values
 *  Hardware CCU of P89LPC93X
 *  V2 2013-05-09
 */

#define minBrightness	0x08		//avoid reduction to very low brightness values by external light
#define maxBrightness	0x7F		//avoid overflow with signed numbers, should be filled with 1 from MSB to LSB
#define DisplayDimCntStart 150		//time until display backlight fades out, max 0xFF
#define startupfocus	1		//focus on back=1 or front=0
#define fadetime		100

unsigned char Brightness[3] = {0,0,0};			//current value
unsigned char Brightness_start[3] = {0x3F,0x3F,0x3F};	//value before lights off
unsigned int ExtBrightness_last = 0x01FFF;			//external brigthness during lights off divided by 256

signed int PWM_set[3] = {0,0,0};				//current pwm value
signed int PWM_incr[3] = {0,0,0};				//pwm dimming step size
unsigned int PWM_incr_cnt[3] = {0,0,0};			//no of steps required to reach targed PWM value

__code char LedNames[3][7] = {"LCD   ","Front ", "Back  "};

void LCD_SendBrightness(unsigned char i)
{
	unsigned int displayvalue;
	displayvalue = (Brightness[i]*202) >> 8;		//scale to 100%
	LCD_SendCmd(LCDSet2ndLine);
	LCD_SendString(&LedNames[i][0]);
	printf_fast("%3d%%        ", displayvalue);
}

void PWM_StepDim()					//perform next dimming step, must frquently called for dimming action
{
	if (PWM_incr_cnt[0])
		{
		PWM_set[0] += PWM_incr[0];
		--PWM_incr_cnt[0];
		OCRAH = (PWM_set[0] >> 6) & 0x00FF;	//with signed int we can not reach 0xFF
		OCRAL = (PWM_set[0] << 2) & 0x00FC;
		}
	if (PWM_incr_cnt[1])
		{
		PWM_set[1] += PWM_incr[1];
		--PWM_incr_cnt[1];
		OCRBH = (PWM_set[1] >> 6) & 0x00FF;
		OCRBL = (PWM_set[1] << 2) & 0x00FC;
		}
	if (PWM_incr_cnt[2])
		{
		PWM_set[2] += PWM_incr[2];
		--PWM_incr_cnt[2];
		OCRCH = (PWM_set[2] >> 6) & 0x00FF;
		OCRCL = (PWM_set[2] << 2) & 0x00FC;
		}
	TCR21 = PLLSetting;	//Set PLL prescaler and start CCU register update
}

void PWM_SetupDim(unsigned char i, signed int PWM_dimsteps, signed char Steps)
{
	signed int temp;
	temp = Brightness[i] + Steps;
	if (maxBrightness < temp)		//avoid overflow
		{
		temp = maxBrightness;
		}
	else if (0 > temp) 
		{
		temp = 0;
		}
	Brightness[i] = temp;

	temp = temp * (temp + 2) - PWM_set[i];
	if ((temp > PWM_dimsteps) || ((temp<0) && (-temp>PWM_dimsteps)))	// if we have more difference then steps to go
		{
		PWM_incr[i] = temp / PWM_dimsteps;
		PWM_set[i] += (temp - PWM_incr[i]*PWM_dimsteps); 		//calc remainder, brackets to avoid overlow!?
		PWM_incr_cnt[i] = PWM_dimsteps;
		}
	else
		{
		if (0<temp)		// if we would have a stepsize smaller then one, we better reduce the number of steps
			{
			PWM_incr[i] = 1;
			PWM_incr_cnt[i] = temp;
			}
		else if (0>temp)
			{
			PWM_incr[i] = -1;
			PWM_incr_cnt[i] = -temp;				//count must be a positive number!
			}
		}
}

void PWM_SetupNow(unsigned char i, signed char Steps)
{
	PWM_SetupDim(i, 1, Steps);
	PWM_StepDim();
}

unsigned int sqrt32(unsigned long a)
{
	unsigned int rem=0;
	unsigned int root=0;
	unsigned int divisor=0;
	int i;

	// Iterate 16 times, because the maximum number of bits in the result is 16 bits
	for(i=0;i<16;i++)
	{
	   	root<<=1;
   		rem=(rem << 2)+(a>>30);
    		a<<=2;
    		divisor=(root<<1)+1;
    		if (divisor<=rem)
	    		{
         		rem-=divisor;
         		root+=1;
     		}
   	}
   	return root;
} 

void SwLightOn(unsigned char i, unsigned int relBrightness)
{
	unsigned long temp;
	temp=Brightness_start[i];
	temp=(temp*relBrightness)>>4;
	if (maxBrightness < temp)						//limit brightness to maximum
		{
		Brightness[i] = maxBrightness;
		}
	else if ((Brightness_start[i]>temp) && (minBrightness>temp))		//limit brigntness ..
		{
		if (minBrightness>Brightness_start[i])
			{
			Brightness[i] = Brightness_start[i];		// .. to last value if it is samller than minimum brightness
			}
		else
			{
			Brightness[i] = minBrightness;			// .. to minimum brightness if the last value was larger than the minimum brightness
			}
		}
	else
		{
		Brightness[i] = temp;					// or just take the calculated value!
		}
	PWM_SetupDim(i, fadetime, 0);
}

void SwLightOff(unsigned char i)
{
	Brightness_start[i]=Brightness[i];
	Brightness[i]=0;
	PWM_SetupDim(i, fadetime, 0);
}

void SwBackLightOn(unsigned char steps)
{
	Brightness[0]=Brightness_start[0];
	PWM_SetupDim(0, steps, 0);
	PWM_StepDim();							// if step = 1 => instant on
	DisplayDimCnt=DisplayDimCntStart;
}

void SwAllLightOn()
{
	unsigned int relBrightness;
	if (0==LightOn)							//remote signal might try to switch a switched on light on again
		{
		FocusBacklight=startupfocus;
		LightOn=1;
		relBrightness=sqrt32(ExtBrightness/ExtBrightness_last);
		SwLightOn(1, relBrightness);
		SwLightOn(2, relBrightness);
		LCD_SendBrightness(startupfocus+1);
		}
}

void SwAllLightOff()
{
	if (1==LightOn)							//remote signal might try to switch a switched on light on again
		{
		LightOn=0;
		Alarmflag=0;
		SwLightOff(1);
		SwLightOff(2);
		ExtBrightness_last=(ExtBrightness>>8) & 0xFFFF;
		if (0==ExtBrightness_last)
			{
			ExtBrightness_last=1;
			}
		SenderMode=Read_EEPROM(EEAddr_SenderMode);				//reset mode
		RefreshTime=1;							//refresh display
		}
}
