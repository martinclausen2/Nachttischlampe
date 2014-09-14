/* Functions to play sound
 * storage for sound: plain 8 bit data stored in program flash
 * internal DAC is used as an output
 * DAC output should be filtered
 * address lines for volume are shared with the get_brightness functions
 *    => disable brightness mesurement during play back and restore settings afterwards
 */

#include "sinetable.c"

#define _CS_Flash		P0_5	//define /Shuntown for audio amp

#define sizesound 6

#define repeatsound 0xFF	//should be no smaller than: volumes steps * 2^5 = 2^2 * 2^5 = 128, other wise max. volume is not reached
			//total duration = 3 * 256 = 768 = ca. 13 minutes

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

// Function to generate sinewaves with different length and frequency using a DDS algorithm
// includes timeout
__bit AcousticDDSAlarm()
{
	unsigned int accu=0;
	unsigned int i;
	unsigned char j;
	unsigned char k;
	__bit user=0;

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
					user=1;
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
	AD1DAT3=sinetable[0];
	return user;
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
		AD1DAT3=sinetable[0];
	}
}

void Beep()
{
	BeepVol(Read_EEPROM(EEAddr_BeepVolume));
}