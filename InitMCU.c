/** Setup MCU for LED Leuchte
 *  V0 2013-01-15
 */

//Devide by last 4 Bit + 1; MSB is start signal
//must reach target frequency of .5 to 1MHz from Osc / 2 
#define PLLSetting  0b10000011

#define MCUIdle 0b00000001

//Disable or Enable RTC with int on external high frequ crystal and clear int flag
#define RTCen	0b01100011
#define RTCdis    0b01100000

void InitMCU()
{
	//Init MCU core
	AUXR1=0b10000000;		//lower power consumption below 8 MHz
	DIVM=0x00;		//run CPU at full speed
	PCONA=0b00100000;		//Disable analog voltage comperators

	//Init port pins
	//Write 1 to port pin if internal control (SPI, UART, ... ) of the port pin is required 

	//P0.0 Quasi-Bi Encoder A
	//P0.1 Quasi-Bi Encoder B
	//P0.2 Quasi-Bi Key A
	//P0.3 Quasi-Bi key B
	//P0.4 Input-Only DAC1 output for speaker
	//     enable analog function with PT0AD.4
	//P0.5 Push-Pull Shutdown Amp LM4861, /CS Flash AT45DB
	//P0.6 Push-Pull Address A Mux 4052
	//P0.7 Push-Pull Address B Mux 4052
	P0M1  = 0b00010000;
	P0M2  = 0b11100000;
	PT0AD = 0b00010000;
	//apply save start-up configuration
	P0    = 0b11111111;

	//P1.0 Quasi-Bi TxD
	//P1.1 Quasi-Bi RxD
	//P1.2 Open-Drain SCL
	//P1.3 Open-Drain SDA
	//P1.4 Quasi-Bi Int from ext RTC
	//P1.5 Input-Only RC5 input
	//P1.6 Open-Drain OCB PWM for Power-LED Driver Spot
	//P1.7 Open-Drain OCC PWM for Power-LED Driver Ambiente
	P1M1 = 0b11101100;
	P1M2 = 0b11001100;
	//apply save start-up configuration
	P1   = 0b00111111;

	//P2.0 Input-Only AD03
	//P2.1 Push-Pull OCD with MOSFET driver
	//P2.2 Push-PUll MOSI
	//P2.3 Quasi-Bi MISO
	//P2.4 Push-Pull SS here /CSB of LCD
	//P2.5 Push-Pull SPICLK
	//P2.6 Push-Pull OCA PWM for Backlight of LCD
	//P2.7 Push-Pull RS of LCD
	P2M1 = 0b00000001;
    	P2M2 = 0b11110110;
	//apply save start-up configuration
	P2   = 0b10111100;

    	//P3 is used for the resonator

	//Init SPI
	//0,1 = 00 = CPUCLK/4 = 1,5MHz (max 3MHz)
	//2 = 1 CLK Phase
	//3 = 1 CLK Polarity
	//4 = 1 Master
	//5 = 0 MSB first
	//6 = 1 Enable SPI
	//7 = 1 Master / Slave defined by Bit 4
	SPCTL = 0b11011100;

	//Init I2C
	//375KHz I2C-CLK at 6MHz (see P89LPC93X User Manual p.86)
	I2SCLH = 4;
	I2SCLL = 4;
	//Enable and operate within external RTC interface, use I2CON

	//Init ADC & DAC
	ADINS  = 0b00001000; //Enable AIN03 for analogue input
	ADCON0 = 0b00000100; //Enable ADC&DAC 0 immediate start
	ADCON1 = 0b00000100; //Enable ADC&DAC 1 immediate start
	ADMODA = 0b00000011; //Fixed channel, single conversion mode for ADC0
	ADMODB = 0b00101000; //ADC Clk  (0,5 to 3,3MHz allowed) Divisor 2 = 3MHz at 6MHz, disable DAC0, enable DAC1
	//Write DAC data to AD1DAT3
	//Write 1 to LSB of ADCON0 to start conversation
	//Read AD0DAT0, 1, 2, 3, 0, ...

	//Init RTC of P89LPC93X
	// config RTC as Timer with 6*10^6/128/25=1875 reload => 25Hz int source
    	RTCH = 0x07; 
    	RTCL = 0x53;
    	RTCCON = RTCdis;
	//enable when configured

    	//switch WTD off or we will be burried under ints
    	WDCON &= 0b11111011; 	//set WD_RUN=0

	//Init CCU
    	TICR2  = 0b00000000;	//disable CCU interrupts
	TPCR2H = 0b00000000;	//CCU prescaler, high byte
	TPCR2L = 0b00000011;	//CCU prescaler, low byte
                             	// Resonator / 2 / (PLLSetting+1) * PLL / (CCU prescaler+1) 
                             	// 6MHz / 2 / 4 * 32 / 4 = 2^16 * 91.55 Hz
	TOR2H = 0xFF;		//Setup reload values for CCU timer, here 2^16-1
    	TOR2L = 0xFF;
    	CCCRA = 0b00000001;	//output compare: non-inverted PWM for LCD backlight
	CCCRB = 0b00000001;	//output compare: non-inverted PWM for power LED
	CCCRC = 0b00000001;	//output compare: non-inverted PWM for power LED
	CCCRD = 0;		//disabled

				//Set pins to make CCU output visible
    	P2_6 = 1;
	P1_6 = 1;    
    	P1_7 = 1;
    	P2_1 = 0;		//stays off for the moment

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
    	TMOD = 0b00000010;	//Init T0 (auto reload timer), T1 (8 bit counter with 1:32 prescaler form 366Hz at 6MHz clk)
    	TCON = 0b01010000;	//gibt T0 & T1 frei
    	TH0 = 256-166;		//T0 high Byte => Reload-Wert
        		        		//clk source is PCLK=CCLK/2, extended by Software with a 1:4 postscaler
	//Init Int
	IP0H = 0b11111011;	//give all except clock on EX1 (IP0X.2) a higher priority, or clock may block encoder
	IP0 = 0b00000000;
	IP1H = 0b11111111;
	IP1 = 0b00000000;

    	IEN0 = 0b11001110;	//Interupt internal RTC (Bit 6) enable, externe RTC on INT1 (Bit 2)
    				//T0 and T1 enable
    	RTCCON = RTCen;
}