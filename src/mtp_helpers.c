/*
 * uMTP Responder
 * Copyright (c) 2018 - 2019 Viveris Technologies
 *
 * uMTP Responder is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * uMTP Responder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMTP Responder; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   mtp_helpers.c
 * @brief  MTP messages creation helpers.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdint.h>
#include "mtp_helpers.h"

void poke(void * buffer, int * index, int typesize, unsigned long data)
{
	unsigned char *ptr;

	ptr = ((unsigned char *)buffer);

	do
	{
		ptr[*index] = data & 0xFF;
		*index += 1;
		data >>= 8;
		typesize--;
	}while( typesize );

	return;
}

uint32_t peek(void * buffer, int index, int typesize)
{
	unsigned char *ptr;
	uint32_t data;
	int shift;

	ptr = ((unsigned char *)buffer);

	shift = 0;
	data = 0x00000000;
	do
	{
		data |= (ptr[index] << shift);
		index++;
		typesize--;
		shift += 8;
	}while( typesize );

	return data;
}

void poke_string(void * buffer, int * index, const char *str)
{
	unsigned char *ptr;
	int i,sizeposition;

	ptr = ((unsigned char *)buffer);

	i = 0;

	// Reserve string size .
	sizeposition = *index;
	ptr[*index] = 0x00;

	*index += 1;

	// Char to unicode...
	while( str[i] )
	{
		ptr[*index] = str[i];
		*index += 1;

		ptr[*index] = 0x00;
		*index += 1;

		i++;
	}

	ptr[*index] = 0x00;
	*index += 1;
	ptr[*index] = 0x00;
	*index += 1;

	i++;

	// Update size position
	ptr[sizeposition] = i;
}

void poke_array(void * buffer, int * index, int size, int elementsize, const unsigned char *bufferin,int prefixed)
{
	unsigned char *ptr;
	int i,nbelement;

	ptr = ((unsigned char *)buffer);

	nbelement = size / elementsize;

	if(prefixed)
	{
		ptr[*index] = nbelement&0xFF;
		*index += 1;
		ptr[*index] = (nbelement>>8)&0xFF;
		*index += 1;
		ptr[*index] = (nbelement>>16)&0xFF;
		*index += 1;
		ptr[*index] = (nbelement>>24)&0xFF;
		*index += 1;
	}

	i = 0;
	while( i < size )
	{
		ptr[*index] = bufferin[i];
		*index += 1;

		i++;
	}
}
