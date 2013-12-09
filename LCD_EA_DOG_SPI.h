/** LCD Type Electronic Assembly DOG-M 
 *  Interface using Hardware SPI of P89LPC93X running at 6MHz and DIVM=0
 */

#ifndef __LCD_H_GUARD
#define __LCD_H_GUARD 1

#define charPerLine 16

#define LCD_CSB P2_4
#define LCDRS P2_7
// LCD3 for 3,3V Display EA DOGM162
#define LCDClearDisplay  0b00000001 //Requires 1,1ms delay
#define LCDReturnHome    0b00000010 //Requires 1,1ms delay
#define LCDDisplayOn     0b00001100 //cursor off
#define LCDCursorOn      0b00001111 //cursor on
#define LCDDisplayOff    0b00001000 //Display off
#define LCDFunctionSet0  0b00111000 //8 bit data length, 2 lines, instruction table 0
#define LCDFunctionSet1  0b00111001 //8 bit data length, 2 lines, instruction table 1
#define LCD3BiasSet      0b00010100 //BS: 1/5, 2 line LCD
#define LCD3PwrControl   0b01010101 //booster on, contrast C5, set c4
#define LCD3FollowerCtrl 0b01101101
#define LCDContrastSet   0b01110000 //zero contrast or with 4 Bit value
#define LCD5BiasSet      0b00011100 //BS: 1/4, 2 line LCD
#define LCD5PwrControl   0b01010010 //booster off, contrast C5, set c4
#define LCD5FollowerCtrl 0b01101001
#define LCDEntryModeSet  0b00000110 //cursor auto increment
#define LCDSetAddress    0b10000000 //or with 7 bit address
#define LCDSet1stLine    0b10000000 //set address to 1st line, 1st char
#define LCDSet2ndLine    0b11000000 //set address to 2nd line, 1st char (address 0x40)

// Waits 1us at 6MHz at a two-clock core 
void Wait2us(unsigned char two_us);

// Waits 100us at 6MHz at a two-clock core
void Wait100us(unsigned char hundred_us);

void LCD_Send(unsigned char Data);

void LCD_SendCmd(unsigned char Command);

void LCD_SendData(unsigned char Data);

void LCD_SetContrast(unsigned char Contrast);

void LCD_ClearDisplay();

void LCD_ReturnHome();

// Init LCD, 3V, Coursor off
void LCD_Init3V();

// Init LCD, 5V, Coursor off
void LCD_Init5V();

#endif
