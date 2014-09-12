/** Handels MotionDetector Input
 designed for 25Hz calling frequency
 */

#define DetectorPort	P1_3

unsigned int MotionDetectorTimer;

void MotionDetector()
{
	unsigned int MotionDetectorMinBrightness;
	if(!DetectorPort && !(LightOn && !MotionDetectorTimer))		//activate only, if light is off or light was switched on by the motion detector
		{
		if (Read_EEPROM(EEAddr_DetectorTimeout))			//active at all?
			{
			MotionDetectorMinBrightness = Read_EEPROM(EEAddr_DetectorBrightness);
			MotionDetectorMinBrightness *= MotionDetectorMinBrightness;
			if (LightOn | (((ExtBrightness>>8) & 0xFFFF) <= MotionDetectorMinBrightness))	   //trigger only if light is on already or if not to bright anyway
				{
				MotionDetectorTimer=(Read_EEPROM(EEAddr_DetectorTimeout)-1)*RTCIntfrequ*60+1;  //DetectorTimeout is here at least 1, 1 is for instandly off (here we need to have one cycle at least)
				SwLightOn();
				#ifdef LCD
				LCD_SendStringFill2ndLine("Motion detected");
				#endif
				}
			}
		}
	else if(1<MotionDetectorTimer)
		{
		MotionDetectorTimer--;
		}
	else if(1==MotionDetectorTimer)
		{
		SwLightOff();
		MotionDetectorTimer=0;
		}
}

unsigned char GetMotionDetectorExtBrightnessValue()
{
	return sqrt32((ExtBrightness>>8) & 0xFFFF);
}