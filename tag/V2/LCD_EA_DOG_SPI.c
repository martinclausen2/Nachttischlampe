/** LCD Type Electronic Assembly DOG-M 
 *  Interface using Hardware SPI of P89LPC93X running at 6MHz and DIVM=0
 *  V0 2013-01-27
 */

#include <p89lpc935_6.h>
#include "LCD_EA_DOG_SPI.h"

// link stdio to LCD
void  putchar(char c) 
{
	LCD_SendData(c);
}

// Waits 2 + 3*N us at 6MHz at a two-clock core
void Wait2us(unsigned char two_us)
{
	unsigned char j;
	for(j=two_us; j; j--){}		//we count down here because it is faster
}

/*				Cycles
	mov	r7,dpl		2
00101$:				0
	mov	a,r7		N*1
	jz	00105$		N*2
	dec	r7		N*1
	sjmp	00101$		N*2
00105$:
	ret			2
		total		4+N*6 = 2 + N*2us @ 6MHz two clock core
*/


// Waits 100us at 6MHz at a two-clock core
void Wait100us(unsigned char hundred_us)
{
	unsigned char j;
	for(j=hundred_us; j; j--){	//we count down here because it is faster
		Wait2us(47);
	}
}

void LCD_Send(unsigned char Data)
{
	LCD_CSB = 0;			//select LCD
	SPDAT = Data;			//send data
	while(0 == (SPSTAT & SPIF)){}	//wait for SPi to finish
	SPSTAT &= ~SPIF;			//reset flag
	Wait2us(14);			//wait 27µs for LCD to execute command
	LCD_CSB = 1;			//release LCD
}

void LCD_SendCmd(unsigned char Command)
{
	LCDRS = 0;
	LCD_Send(Command);
}

void LCD_SendData(unsigned char Data)
{
	LCDRS = 1;			//select data register
	LCD_Send(Data);
}

void LCD_ClearDisplay()
{
	LCD_SendCmd(LCDClearDisplay);
	Wait100us(11);
}

void LCD_ReturnHome()
{
	LCD_SendCmd(LCDReturnHome);
	Wait100us(11);
}

void LCD_SetContrast(unsigned char Contrast)
{
	LCD_SendCmd(LCDFunctionSet1);
	Contrast&=0x0f;		//use only lower bits
	Contrast|=LCDContrastSet;	//add command
	LCD_SendCmd(Contrast);
	LCD_SendCmd(LCDFunctionSet0);
}

void LCD_SendString(__code unsigned char *var)
{
     while(*var)			//till string ends
	{
     	LCD_SendData(*var++);	//send characters one by one
     	}
}


// Init LCD, for 3V, Coursor off
void LCD_Init3V()
{
	LCD_CSB = 1;
	Wait100us(200);
	Wait100us(200);
	LCD_SendCmd(LCDFunctionSet1);
	LCD_SendCmd(LCD3BiasSet);
	LCD_SendCmd(LCD3PwrControl);
	LCD_SendCmd(LCD3FollowerCtrl);
	LCD_SetContrast(0b00001000);
	LCD_SendCmd(LCDFunctionSet0);
	LCD_SendCmd(LCDDisplayOn);
	LCD_ClearDisplay();
	LCD_SendCmd(LCDEntryModeSet);
}

// Init LCD, for 5V, Coursor off
void LCD_Init5V()
{
	LCD_CSB = 1;
	Wait100us(200);
	Wait100us(200);
	LCD_SendCmd(LCDFunctionSet1);
	LCD_SendCmd(LCD5BiasSet);
	LCD_SendCmd(LCD5PwrControl);
	LCD_SendCmd(LCD5FollowerCtrl);
	LCD_SetContrast(0b00000100);
	LCD_SendCmd(LCDFunctionSet0);
	LCD_SendCmd(LCDDisplayOn);
	LCD_ClearDisplay();
	LCD_SendCmd(LCDEntryModeSet);
}
