/*
 * num2str.c
 *
 *  Created on: Mar 21, 2020
 *      Author: roberto-mint
 */

void hex2str(char tstr[], int value, int slen)
{
	int i;
	for(i=slen-1;i>=0;i--)
	{
		tstr[i] = 0x30 | (value & 0xf);
		value >>=4;
		if (tstr[i] > 0x39)
			tstr[i]+=7;
	}
	tstr[slen]=0;
}

void dec2str(char tstr[], int value, int slen, int ndec)
{
	int i,digit;
	ndec=slen-ndec; // decimal point position
	for(i=slen;i>=0;i--)
	{
		if (i== ndec)
			tstr[i-1] = '.';
		else
		{
			digit = value %10;
			if(value || i> ndec)
				tstr[i-1] = 0x30 | digit;
			else
				tstr[i-1] = ' ';
			value /=10;
		}
	}
	tstr[slen]=0;
}


