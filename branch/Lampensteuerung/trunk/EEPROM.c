/**
 * EEPROM access
 * @file EEPROM.c
 */

#include <p89lpc935_6.h>

unsigned char Read_EEPROM(unsigned char address)
{
	DEECON=0;		//clear EEIF, Byte read / write, first page
	DEEADR=address;
	while( 0 == (DEECON & EEIF) ){}
	return DEEDAT;
}

void Write_EEPROM(unsigned char address, unsigned char data2write)
{
	if (Read_EEPROM(address)!=data2write)	//write only if content has changed!
		{
		DEECON=0;	//clear EEIF, Byte read / write, first page
		DEEDAT=data2write;
		DEEADR=address;
		while( 0 == (DEECON & EEIF) ){}
		}
}