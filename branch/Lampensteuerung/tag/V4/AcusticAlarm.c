/* Functions to play sound
 * storage for sound: plain 8 bit data stored in external data flash connected to spi port
 *    => do no access flash and lcd simultaniously!
 * /cs of flash and shutdown of amplifier can share the same control line
 * internal DAC is used as an output
 * DAC output should be filtered
 * address lines for volume are shared with the get_brightness functions
 *    => disable brightness mesurement during play back and restore settings afterwards
 *
 * Functions:
 * setup menu, accessable during start up
 * external flash chip erase followed by 
 * loading external data flash on spi port with data from uart
 *
 * V0 2013-01-10
 */

// Function to generate sinewaves with different length and frequency using a DDS algorithm

#include "sinetable.c"

#define sizesound 6

#define repeatsound 0x7F

__code unsigned int sound[sizesound][2] = {{65000, 700},{45000, 950},{35000, 750},{25000, 1150},{65000, 0},{65000, 0}};

#define BeepLength 20000

#define BeepFrequency 750

	typedef union {
		struct {
			unsigned ALL:8;  
		};
		struct {
			unsigned LSB:1; 
			unsigned MSB:1;
			unsigned free:6;
		};
	} AudioGain_t;

void AcusticDDSAlarm()
{
	unsigned int accu=0;
	unsigned int i;
	unsigned char j;
	unsigned char k=0;

	AudioGain_t AudioGain;

	EA=0;
	_CS_Flash=0;
	for(k=0; repeatsound>k; k++)
		{
		for(j=0; sizesound>j; j++)
			{
			AudioGain.ALL=~(k>>5);
			P0_6 = AudioGain.LSB;
			P0_7 = AudioGain.MSB;
			for(i=sound[j][0]; i; i--)
				{
				accu+=sound[j][1];
				AD1DAT3=sinetable[((accu>>8) & 0xFF)];
				if(~KeysPort & SelKeys)
					{
					goto endofalarm;
					}
				}
			}
		}
	endofalarm:
	_CS_Flash=1;
	P0_6 = PhotoGain.LSB;	//restore gain
	P0_7 = PhotoGain.MSB;
	EA=1;
}

void BeepVol(unsigned char Volume)
{
	unsigned int accu=0;
	unsigned int i;

	AudioGain_t AudioGain;

	AudioGain.ALL=Volume;
	if (0!=AudioGain.ALL)
	{
		--AudioGain.ALL;
		AudioGain.ALL=~AudioGain.ALL;
		EA=0;
		_CS_Flash=0;
		P0_6 = AudioGain.LSB;
		P0_7 = AudioGain.MSB;
		for(i=BeepLength; i; i--)
			{
			accu+=BeepFrequency;
			AD1DAT3=sinetable[((accu>>8) & 0xFF)];
			}
		_CS_Flash=1;
		P0_6 = PhotoGain.LSB;	//restore gain
		P0_7 = PhotoGain.MSB;
		EA=1;
	}
}

void Beep()
{
	BeepVol(Read_EEPROM(EEAddr_BeepVolume));
}