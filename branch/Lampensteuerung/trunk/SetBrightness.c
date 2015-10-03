/** Sets out PWM value for LampenSteuerung
 *  Hardware CCU of P89LPC93X
 */

void PWM_Set()
{
	unsigned int temp;
	if (PWM_setlimited)
		{
		temp = (unsigned int)PWM_setlimited + PWM_Offset;
		if (maxRawPWM<temp)
			{
			temp=maxRawPWM;
			}
		// add min pulse width => we do not use full 16 bit resolution
		// & reach PWM = 100% on for Brightness = 0x7F
		OCRDH =  ((temp >> 6) & 0x00FF);
		OCRDL = (((temp << 2) & 0x00FC) | 0x0003);
		}
	else
		{
		OCRDH = 0;
		OCRDL = 0;
		}
	AD1DAT3 = OCRDH;		//set analogue output
	TCR21 = PLLSetting;	//Set PLL prescaler and start CCU register update
}

void LCD_SendBrightness()
{
	unsigned int displayvalue;
	displayvalue = (Brightness*202) >> 8;		//scale to 100%
	#ifdef LCD
	LCD_SendString2ndLine("On ");
	printf_fast("%3d%%        ", displayvalue);
	#endif
}

void SendBrightness()
{
	ADMODB = DAC1;
	PWM_Set();	//importent order: first DAC on then imedeatly wirte value to avoid spike on output
	LightOn=1;
	LCD_SendBrightness();
	LEDOn();
	DAC1Port = 1;
}

void Update_PWM_Offset()
{
	PWM_Offset = Read_EEPROM(EEAddr_OffsetFrontBrightness);
	PWM_Offset *= PWM_Offset;
}

void InitBrightness()
{
	Update_PWM_Offset();

	Brightness_start=Read_EEPROM(EEAddr_CurrentBrightness);
	ExtBrightness_last=Read_EEPROM(EEAddr_ExtBrightness_last_MSB)<<8;
	ExtBrightness_last+=Read_EEPROM(EEAddr_ExtBrightness_last_LSB);
	InitMeasureExtBrightness();
}

void SetExtBrightness_last()
{
	ExtBrightness_last=(ExtBrightness>>8) & 0xFFFF;
	if (0==ExtBrightness_last)
		{
		ExtBrightness_last=1;
		}
}

void StoreBrightness()
{
	if (1<WriteTimer)		//store current brightness after timeout
		{
		--WriteTimer;
		}
	else if (1 == WriteTimer)
		{
		if (LightOn)
			{
			Write_EEPROM(EEAddr_CurrentBrightness, Brightness);
			SetExtBrightness_last();
			Write_EEPROM(EEAddr_ExtBrightness_last_MSB, (ExtBrightness_last>>8) & 0xFF);
			Write_EEPROM(EEAddr_ExtBrightness_last_LSB, ExtBrightness_last & 0xFF);
			}
		WriteTimer=0;
		}
}

void PWM_StepDim()		// perform next dimming step, must frquently called for dimming action
{
	if (PWM_incr_cnt)
		{
		PWM_set += PWM_incr;
		--PWM_incr_cnt;
		}
}

void PWM_SetupDim(signed int PWM_dimsteps, signed char Steps, signed int minBrightness)
{
	signed int temp;
	limit=0;
	temp = Brightness + Steps;
	if (maxBrightness < temp)		//avoid overflow
		{
		temp = maxBrightness;
		limit = maxLimit;
		}
	else if (minBrightness > temp) 
		{
		temp = minBrightness;
		limit = minLimit;
		}
	Brightness = temp;

	temp = temp * (temp + 2) - PWM_set;
	if ((temp > PWM_dimsteps) || ((temp<0) && (-temp>PWM_dimsteps)))	// if we have more difference then steps to go
		{
		PWM_incr = temp / PWM_dimsteps;
		PWM_set += (temp - PWM_incr*PWM_dimsteps); 			//calc remainder, brackets to avoid overlow!?
		PWM_incr_cnt = PWM_dimsteps;
		}
	else
		{
		if (0<temp)		// if we would have a stepsize smaller then one, we better reduce the number of steps
			{
			PWM_incr = 1;
			PWM_incr_cnt = temp;
			}
		else if (0>temp)
			{
			PWM_incr = -1;
			PWM_incr_cnt = -temp;				//count must be a positive number!
			}
		}
}

void PWM_SetupNow(signed char Steps)
{
	PWM_SetupDim(1, Steps, 0);
	PWM_StepDim();
	LimitOutput();
	PWM_Set();
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

void SwLightOn()
{
	unsigned int relBrightness;
	unsigned long temp;
	unsigned char minBrightness;				//avoid reduction to very low brightness values by external light

	if (0==LightOn)				  		//remote signal might try to switch a switched on light on again
		{
		relBrightness=sqrt32(ExtBrightness/ExtBrightness_last);

		minBrightness = Read_EEPROM(EEAddr_MinimumFrontBrightness);
		temp=Brightness_start;
		temp=(temp*relBrightness)>>4;
		if (maxBrightness < temp)					//limit brightness to maximum
			{
			Brightness = maxBrightness;
			}
		else if ((Brightness_start>temp) && (minBrightness>temp))	//limit brigntness ..
			{
			if (minBrightness>Brightness_start)
				{
				Brightness = Brightness_start;		// .. to last value if it is smaller than minimum brightness
				}
			else
				{
				Brightness = minBrightness;		// .. to minimum brightness if the last value was larger than the minimum brightness
				}
			}
		else
			{
			Brightness = temp;				// or just take the calculated value!
			}
		PWM_SetupDim(fadetime, 0, 0);
		LimitOutput();
		SendBrightness();
		}
}

void SwLightOff()
{
	if (1==LightOn)						//remote signal might try to switch a switched on light on again
		{
		LightOn=0;
		Alarmflag=0;
		Brightness_start=Brightness;
		Brightness=0;
		PWM_SetupDim(fadetime, 0, 0);
		SetExtBrightness_last();
		#ifdef LCD
		LCD_SendStringFill2ndLine("Standby");
		#endif
		LEDStandby();
		}
}

void SwLightOnMax()
{
	Brightness = maxBrightness;
	PWM_SetupDim(fadetime, 0, 0);
	SendBrightness();
}

void SwLightOnMin()
{
	Brightness = Read_EEPROM(EEAddr_MinimumFrontBrightness);
	PWM_SetupDim(fadetime, 0, 0);
	SendBrightness();
}