/** Setup MCU for LampenSteuerung

DAC and ADC can NOT be used at the same time, also if the DAC output is filtered at a very low frequency!!!

 */

//Devide by last 4 Bit + 1; MSB is start signal
//must reach target frequency of .5 to 1MHz from Osc / 2 
#define PLLSetting  0b10000011

#define MCUIdle	0b00000001

//Disable or Enable RTC with int on external high frequ crystal and clear int flag
#define RTCen	0b01100011
#define RTCdis    0b01100000
#define RTCIntfrequ	10	//frequency at which RTC timer generates an interrupt
				//adjust RTC reload value

#define P0M1def   0b00010011

#define DAC1	0b01001000	//ADC Clk  (0,5 to 3,3MHz allowed) Divisor 3 disable DAC0,  enable DAC1
#define ADC1	0b01000000	//ADC Clk  (0,5 to 3,3MHz allowed) Divisor 3 disable DAC0, disable DAC1

#define DAC1Port  P0_4		//should be pulled low by an open drain output if not controlled by the DAC

void InitMCU()
{
	//Init MCU core
	AUXR1=0b10000000;		//lower power consumption below 8 MHz
	DIVM=0x00;		//run CPU at full speed
	PCONA=0b00101010;		//Disable analog voltage comperators, I2C, UART

	//Init port pins
	//Write 1 to port pin if internal control (SPI, UART, ... ) of the port pin is required 

	//P0.0 Input-Only ADC Temp 
	//P0.1 Input-Only ADC Brightness
	//P0.2 Quasi-Bi NC
	//P0.3 Quasi-Bi NC
	//P0.4 Open-Drain DAC1 for output, pull down if DAC1 is off during standby
	//P0.5 Push-Pull or Open-Drain Low Bias for phototransistor
	//P0.6 Push-Pull or Open-Drain Medium Bias for phototransistor
	//P0.7 Push-Pull or Open-Drain High Bias for phototransistor
	P0M1  = P0M1def;
	P0M2  = 0b00010000;

	//apply save start-up configuration
	P0    = 0b11101111;

	//P1.0 Quasi-Bi TxD
	//P1.1 Quasi-Bi RxD
	//P1.2 Open-Drain SCL
	//P1.3 Input only Motion Detector Input (no quasi-bi possible) SDA
	//P1.4 Quasi-Bi RC5 input
	//P1.5 Input only (no configuration possible) Key
	//P1.6 Push-Pull OCB PWM for status LED
	//P1.7 Push-Pull OCC PWM for status LED 
	P1M1 = 0b00101100;
	P1M2 = 0b11000100;
	//apply save start-up configuration
	P1   = 0b11111111;

	//P2.0 Quasi-Bi NC
	//P2.1 Push-Pull OCC PWM for status LED
	//P2.2 Push-PUll MOSI
	//P2.3 Quasi-Bi MISO
	//P2.4 Push-Pull SS here /CSB of LCD
	//P2.5 Push-Pull SPICLK
	//P2.6 Push-Pull OCA PWM for output
	//P2.7 Push-Pull RS of LCD
	P2M1 = 0b00000000;
    	P2M2 = 0b11110110;
	//apply save start-up configuration
	P2   = 0b11111111;

    	//P3 is unused

	#ifdef LCD
	//Init SPI
	//0,1 = 00 = CPUCLK/4 = 1,5MHz (max 3MHz)
	//2 = 1 CLK Phase
	//3 = 1 CLK Polarity
	//4 = 1 Master
	//5 = 0 MSB first
	//6 = 1 Enable SPI
	//7 = 1 Master / Slave defined by Bit 4
	SPCTL = 0b11011100;
	#endif

	//Init ADC & DAC
	ADINS  = 0b00010010; //Enable AIN01 and AIN10 for analogue input
	ADCON0 = 0b00000100; //Enable ADC&DAC 0 immediate start
	ADCON1 = 0b00000100; //Enable ADC&DAC 1 immediate start
	ADMODA = 0b00110011; //Fixed channel, single conversion mode for ADC0 & ADC1
	ADMODB = ADC1;
	//Write DAC data to AD1DAT3
	//Write 1 to LSB of ADCON0 to start conversation
	//Read AD0DAT0, 1, 2, 3, 0, ...

	//Init RTC of P89LPC93X
	// config RTC as Timer with 7.373*10^6/128/40=1440 reload => 40Hz int source
    	RTCH = 0x05; 
    	RTCL = 0xA0;
    	RTCCON = RTCdis;
	//enable when configured

    	//switch WTD off or we will be burried under ints
    	WDCON &= 0b11111011; 	//set WD_RUN=0

	//Init CCU
    	TICR2  = 0b00000000;	//disable CCU interrupts
	TPCR2H = 0b00000000;	//CCU prescaler, high byte
                             	// Resonator / 2 / (PLLSetting+1) * PLL / (CCU prescaler+1) 
                             	// 7.373MHz / 2 / 4 * 32 / 4 = 2^16 * 112.5 Hz
	TPCR2L = 0b00000011;	//CCU prescaler, low byte


	TOR2H = 0xFF;		//Setup reload values for CCU timer, here 2^16-1
    	TOR2L = 0xFF;
    	CCCRA = 0b00000001;	//output compare: non-inverted PWM for status LED
	CCCRB = 0b00000001;	//output compare: non-inverted PWM for status LED
	CCCRC = 0b00000001;	//output compare: non-inverted PWM for status LED
	CCCRD = 0b00000011;	//output compare: inverted PWM for output

				//Set pins to make CCU output visible
    	P2_6 = 1;		//blue LED
	P1_6 = 1;		//green LED
    	P1_7 = 1;		//red LED
    	P2_1 = 1;		//Output

    	//start PLL
         TCR21 = PLLSetting;	//Set PLL prescaler and start CCU register update 
         PLEEN = 1;         	//Enable PLL
	while(0==PLEEN){}	//wait for PLL lockin

	OCRAH = 0;		//Reset PWM values
	OCRAL = 0;
	OCRBH = 0;
	OCRBL = 0;
	OCRCH = 0;
	OCRCL = 0;
	OCRDH = 0;
	OCRDL = 0;

    	TCR20 = 0b10000110;	//Enable Asymmetrical PWM with down counting    

	//Setup Timer 0 and Timer 1

    	// config Timer 0 for RC5 receiver
    	// config Timer 1 for encoder decoding
    	TMOD = 0b00000010;	//Init T0 (auto reload timer), T1 (8 bit counter with 1:32 prescaler form 450Hz at 7,373MHz clk)
    	TCON = 0b01010000;	//gibt T0 & T1 frei
    	TH0 = 256-204;		//T0 high Byte => Reload-Wert
        			       	//clk source is PCLK=CCLK/2, extended by Software with a 1:4 postscaler, effective frequency 4518Hz
	//Init Int
    	IEN0 = 0b11001010;	//Interupt internal RTC (Bit 6) enable
    				//T0 and T1 enable
    	RTCCON = RTCen;
}