/** Sets all PWM values
 *  Hardware CCU of P89LPC93X
 */

#define maxBrightness	0x7F		//avoid overflow with signed numbers, should be filled with 1 from MSB to LSB
#define maxRawPWM		0x3FFF		// =  maxBrigntess^2
#define DisplayDimCntStart 250		//time until display backlight fades out, max 0xFF
#define startupfocus	1		//focus on back=1 or front=0
#define fadetime		150

unsigned char Brightness[3] = {0,0,0};			//current value
unsigned char Brightness_start[3] = {0x3F,0x3F,0x3F};	//value before lights off
unsigned int PWM_Offset[3] = {0,0,0};			//PWM value, where the driver effectivly starts to generate an output
unsigned int ExtBrightness_last = 0x01FFF;			//external brigthness during lights off divided by 256

signed int PWM_set[3] = {0,0,0};				//current pwm value
signed int PWM_incr[3] = {0,0,0};				//pwm dimming step size
unsigned int PWM_incr_cnt[3] = {0,0,0};			//no of steps required to reach targed PWM value

__code char LedNames[3][7] = {"LCD   ","Front ", "Back  "};

void LCD_SendBrightness(unsigned char i)
{
	unsigned int displayvalue;
	displayvalue = (Brightness[i]*202) >> 8;		//scale to 100%
	LCD_SendString2ndLine(&LedNames[i][0]);
	printf_fast("%3d%%        ", displayvalue);
}

void Update_PWM_Offset(unsigned char i)
{
	if (0==i)
		{
		PWM_Offset[i]=0;				//0 is lcd backlight
		}
	else
		{
		PWM_Offset[i]  = Read_EEPROM(EEAddr_OffsetFrontBrightness+i-1);
		PWM_Offset[i] *= PWM_Offset[i];
		}
}

void PWM_StepDim()		// perform next dimming step, must frquently called for dimming action
				// contains repeated code to optimize performance
{
	unsigned int temp;
	if (PWM_incr_cnt[0])
		{
		PWM_set[0] += PWM_incr[0];
		--PWM_incr_cnt[0];
		if (PWM_set[0])
			{
			// add min pulse width => we do not use full 16 bit resolution
			// & reach PWM = 100% on for Brightness = 0x7F
			OCRAH =  ((PWM_set[0] >> 6) & 0x00FF);
			OCRAL = (((PWM_set[0] << 2) & 0x00FC) | 0x0003);
			}
		else
			{
			OCRAH = 0;
			OCRAL = 0;
			}
		}

	if (PWM_incr_cnt[1])
		{
		PWM_set[1] += PWM_incr[1];
		--PWM_incr_cnt[1];
		if (PWM_set[1])
			{
			// with signed int we can not reach 0xFFFF, only 0x3FFF is posssible
			// including the offset we reach max 0x7FFF, which is just clipped to 0x3FFF
			temp = (unsigned int)PWM_set[1] + PWM_Offset[1];
			if (maxRawPWM<temp)
				{
				temp=maxRawPWM;
				}
			OCRBH =  ((temp >> 6) & 0x00FF);
			OCRBL = (((temp << 2) & 0x00FC) | 0x0003);
			}
		else
			{
			OCRBH = 0;
			OCRBL = 0;
			}
		}

	if (PWM_incr_cnt[2])
		{
		PWM_set[2] += PWM_incr[2];
		--PWM_incr_cnt[2];
		if (PWM_set[2])
			{
			temp = (unsigned int)PWM_set[2] + PWM_Offset[2];
			if (maxRawPWM<temp)
				{
				temp=maxRawPWM;
				}
			OCRCH =  ((temp >> 6) & 0x00FF);
			OCRCL = (((temp << 2) & 0x00FC) | 0x0003);
			}
		else
			{
			OCRCH = 0;
			OCRCL = 0;
			}
		}

	// adjust PWM frequency to optain a wider linear range of the PWM
	// find larger PWM value
	if (PWM_set[1]>PWM_set[2])
		{
		temp=PWM_set[1];
		}
	else
		{
		temp=PWM_set[2];
		}

	// Resonator / 2 / (PLLSetting+1) * PLL / (CCU prescaler+1) 
	// 6MHz / 2 / 4 * 32 / 1 = 2^16 * 366.2 Hz
	// TPCR2L CCU prescaler, low byte

#if HighPWM == 1
	if (temp>800)
		{
		TPCR2L=0;	//366Hz
		}
	else if (temp>400)
		{
		TPCR2L=1;	//183Hz
		}
	else
		{
		TPCR2L=2;	//122Hz
		}
	//at lower rates the ZXLD1374 is not switching on
#else
	if (temp>800)
		{
		TPCR2L=0;	//366Hz
		}
	else if (temp>400)
		{
		TPCR2L=1;	//183Hz
		}
	else if (temp>200)
		{
		TPCR2L=2;	//122Hz
		}
	else if (temp>100)
		{
		TPCR2L=3;	//91.6Hz
		}
	else if (temp>50)
		{
		TPCR2L=4;	//73.2Hz
		}
	else
		{
		TPCR2L=5;	//61.0Hz
		}
	// lower rates are flickering to much and lead to aliasing effects with the LCD
#endif

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
	unsigned char minBrightness;					//avoid reduction to very low brightness values by external light

	minBrightness = Read_EEPROM(EEAddr_MinimumFrontBrightness+i-1);	//0 is lcd backlight
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

void SwBackLightOn(signed int steps)
{
	if (!DisplayDimCnt)						// do not call twice while active to avoid instand light on
		{
		Brightness[0]=Brightness_start[0];
		PWM_SetupDim(0, steps, 0);
		PWM_StepDim();						// if step = 1 => instant on
		}
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
		SenderMode=Read_EEPROM(EEAddr_SenderMode);			//reset mode
		RefreshTime=1;						//refresh display
		}
}


// better than repetive code above in void PWM_StepDim(), but could not make it work (ligth stays off)


void PWM_CalcSet(__near unsigned char *high, __near unsigned char *low, unsigned char i)
{
	unsigned int temp;
	if (PWM_incr_cnt[i])
		{
		PWM_set[i] += PWM_incr[i];
		--PWM_incr_cnt[i];
		if (PWM_set[i])
			{
			//with signed int we can not reach 0xFFFF, only 0x3FFF in posssible
			// including the offset we reach max 0x7FFF, which is just clipped to 0x3FFF
			temp = (unsigned int)PWM_set[i] + PWM_Offset[i];
			if (maxRawPWM<temp)
				{
				temp=maxRawPWM;
				}
			*high = (unsigned char) ((temp >> 6) & 0x00FF);
			*low  = (unsigned char) (((temp << 2) & 0x00FC) | 0x0003);	//add min pulse width => we do not use full 16 bit resolution & reach PWM = 100% on for Brightness = 0x7F
			}
		else
			{
			*high = (unsigned char) 0;
			*low = (unsigned char) 0;
			}
		}
}
